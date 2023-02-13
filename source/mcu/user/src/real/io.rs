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
    pos: usize,
}

impl Stdout {
    pub fn lock(&self) -> StdoutLock<'static> {
        StdoutLock {
            guard: STDOUT.lock(Duration::infinite()).unwrap(),
            pos: 0,
        }
    }
}

struct LfToCrLf<I: Iterator<Item = u8>> {
    iter: I,
    lf: bool,
}

impl<I: Iterator<Item = u8>> LfToCrLf<I> {
    fn new(iter: I) -> Self {
        Self { iter, lf: false }
    }
}

impl<I: Iterator<Item = u8>> Iterator for LfToCrLf<I> {
    type Item = u8;
    fn next(&mut self) -> Option<u8> {
        if self.lf {
            self.lf = false;
            return Some(b'\n');
        }
        let b = self.iter.next()?;
        if b == b'\n' {
            self.lf = true;
            return Some(b'\r');
        }
        Some(b)
    }
}

impl<'a> StdoutLock<'a> {
    unsafe fn push_byte_unchecked(&mut self, b: u8) {
        *self.guard.buffer().get_unchecked_mut(self.pos) = b;
        self.pos += 1;
    }
    fn write_byte(&mut self, b: u8) {
        unsafe { self.push_byte_unchecked(b) };
        if self.pos >= GlobalStdout::buffer_len() {
            self.guard.write_buffer();
            self.pos = 0;
        }
    }
    fn flush(&mut self) {
        if self.pos > 0 {
            unsafe { self.push_byte_unchecked(0) };
            self.guard.write_buffer();
            self.pos = 0;
        }
    }
}

impl<'a> Write for StdoutLock<'a> {
    fn write_str(&mut self, s: &str) -> Result<(), Error> {
        let src = LfToCrLf::new(s.as_bytes().iter().cloned());
        for b in src {
            self.write_byte(b);
        }
        self.flush();
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

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
        use core::{write, fmt::Write};
        write!($crate::io::stdout(), $($arg)*).unwrap();
    }};
}

#[macro_export]
macro_rules! println {
    () => {{
        $crate::print!("\n");
    }};
    ($($arg:tt)*) => {{
        use core::{write, fmt::Write};
        let mut stdout = $crate::io::stdout().lock();
        write!(stdout, $($arg)*).and_then(|()| write!(stdout, "\n")).unwrap();
    }};
}

pub use print;
pub use println;
