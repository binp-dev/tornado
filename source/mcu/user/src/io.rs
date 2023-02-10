use core::{ffi::c_char, slice::from_raw_parts_mut};
use freertos::{self, Duration, Mutex};
use lazy_static::lazy_static;

pub use core::fmt::{Error, Write};

extern "C" {
    static __hal_io_buffer_size: usize;
    static __hal_io_buffer: [c_char; 0];

    fn __hal_print_buffer();
}

lazy_static! {
    static ref STDOUT: Mutex<GlobalStdout> = Mutex::new(GlobalStdout::new()).unwrap();
}

struct GlobalStdout {}

impl GlobalStdout {
    fn new() -> Self {
        Self {}
    }
    fn buffer_len() -> usize {
        unsafe { __hal_io_buffer_size }
    }
    fn buffer(&mut self) -> &mut [u8] {
        unsafe { from_raw_parts_mut(__hal_io_buffer.as_ptr() as *mut u8, Self::buffer_len()) }
    }
    fn write_buffer(&mut self) {
        unsafe { __hal_print_buffer() }
    }
}

pub struct Stdout {
    _unused: [u8; 0],
}

type MutexGuard<'a, T> = freertos::MutexGuard<'a, T, freertos::MutexNormal>;

pub struct StdoutLock<'a> {
    guard: MutexGuard<'a, GlobalStdout>,
}

impl Stdout {
    pub fn lock(&self) -> StdoutLock<'static> {
        StdoutLock {
            guard: STDOUT.lock(Duration::infinite()).unwrap(),
        }
    }
}

impl<'a> Write for StdoutLock<'a> {
    fn write_str(&mut self, s: &str) -> Result<(), Error> {
        let mut src = s.as_bytes();
        let buf_len = GlobalStdout::buffer_len();
        while src.len() > buf_len {
            self.guard.buffer().copy_from_slice(&src[..buf_len]);
            src = &src[buf_len..];
            self.guard.write_buffer();
        }
        let dst = self.guard.buffer();
        dst[..src.len()].copy_from_slice(src);
        if src.len() < buf_len {
            dst[src.len()] = 0;
        }
        self.guard.write_buffer();
        Ok(())
    }
}

impl Write for Stdout {
    fn write_str(&mut self, s: &str) -> Result<(), Error> {
        self.lock().write_str(s)
    }
}

pub fn stdout() -> Stdout {
    Stdout { _unused: [] }
}

macro_rules! print {
    ($($arg:tt)*) => {{
        use core::{write, fmt::Write};
        write!($crate::io::stdout(), $($arg)*).unwrap();
    }};
}

macro_rules! println {
    () => {{
        use core::{write, fmt::Write};
        write!($crate::io::stdout(), "\r\n").unwrap();
    }};
    ($($arg:tt)*) => {{
        use core::{write, fmt::Write};
        let mut stdout = $crate::io::stdout().lock();
        write!(stdout, $($arg)*).and_then(|()| write!(stdout, "\r\n")).unwrap();
    }};
}
