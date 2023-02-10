use super::Channel;
use futures::{
    io::{AsyncRead, AsyncWrite},
    task::AtomicWaker,
};
use mio::{self, unix::SourceFd, Events, Interest, Token};
use std::{
    fs::{File, OpenOptions},
    io,
    os::{
        fd::{FromRawFd, IntoRawFd, RawFd},
        raw::c_void,
        unix::fs::OpenOptionsExt,
    },
    path::Path,
    pin::Pin,
    sync::{
        atomic::{AtomicUsize, Ordering},
        Arc,
    },
    task::{Context, Poll},
    thread,
};
use termios::Termios;

const WAKER_TOKEN: Token = Token(0);
const FD_TOKEN: Token = Token(1);

struct Shared {
    fd: RawFd,
    read_waker: AtomicWaker,
    write_waker: AtomicWaker,
    poll_waker: mio::Waker,
    uses: AtomicUsize,
}

pub struct Rpmsg {
    shared: Arc<Shared>,
}

impl Rpmsg {
    pub async fn open<P: AsRef<Path>>(path: P) -> Result<Self, io::Error> {
        let fd = {
            let file = OpenOptions::new()
                .read(true)
                .write(true)
                .custom_flags(libc::O_NOCTTY | libc::O_NONBLOCK)
                .open(path)?;
            file.into_raw_fd()
        };
        {
            let mut tty = Termios::from_fd(fd)?;
            termios::cfmakeraw(&mut tty);
            termios::tcsetattr(fd, termios::TCSAFLUSH, &tty)?;
        }

        let poll = mio::Poll::new()?;

        let shared = Arc::new(Shared {
            fd,
            read_waker: AtomicWaker::new(),
            write_waker: AtomicWaker::new(),
            poll_waker: mio::Waker::new(poll.registry(), WAKER_TOKEN)?,
            uses: AtomicUsize::new(1),
        });

        {
            let shared = shared.clone();
            thread::spawn(move || shared.run(poll));
        }

        Ok(Rpmsg { shared })
    }
}

impl Channel for Rpmsg {
    type Read = Reader;
    type Write = Writer;

    fn split(self) -> (Reader, Writer) {
        self.shared.uses.fetch_add(2, Ordering::SeqCst);
        (
            Reader {
                shared: self.shared.clone(),
            },
            Writer {
                shared: self.shared.clone(),
            },
        )
    }
}

impl Shared {
    fn run(self: Arc<Self>, mut poll: mio::Poll) {
        let fd = self.fd;
        let mut source_fd = SourceFd(&fd);
        poll.registry()
            .register(&mut source_fd, FD_TOKEN, Interest::READABLE | Interest::WRITABLE)
            .unwrap();

        let mut events = Events::with_capacity(3);
        loop {
            poll.poll(&mut events, None).unwrap();
            for event in events.iter() {
                match event.token() {
                    WAKER_TOKEN => {
                        if self.uses.load(Ordering::SeqCst) == 0 {
                            break;
                        }
                    }
                    FD_TOKEN => {
                        if event.is_readable() {
                            self.read_waker.wake();
                        }
                        if event.is_writable() {
                            self.write_waker.wake();
                        }
                    }
                    _ => unreachable!(),
                }
            }
        }
    }

    unsafe fn read(&self, dst: &mut [u8]) -> io::Result<usize> {
        let r = unsafe { libc::read(self.fd, dst.as_ptr() as *mut c_void, dst.len()) };
        if r < 0 {
            Err(io::Error::last_os_error())
        } else {
            Ok(r as usize)
        }
    }

    unsafe fn write(&self, src: &[u8]) -> io::Result<usize> {
        let r = unsafe { libc::write(self.fd, src.as_ptr() as *const c_void, src.len()) };
        if r < 0 {
            Err(io::Error::last_os_error())
        } else {
            Ok(r as usize)
        }
    }
}

impl Drop for Shared {
    fn drop(&mut self) {
        unsafe { File::from_raw_fd(self.fd) };
    }
}

pub struct Reader {
    shared: Arc<Shared>,
}

pub struct Writer {
    shared: Arc<Shared>,
}

impl Drop for Rpmsg {
    fn drop(&mut self) {
        self.shared.uses.fetch_sub(1, Ordering::SeqCst);
        self.shared.poll_waker.wake().unwrap();
    }
}

impl Drop for Reader {
    fn drop(&mut self) {
        self.shared.uses.fetch_sub(1, Ordering::SeqCst);
        self.shared.poll_waker.wake().unwrap();
    }
}

impl Drop for Writer {
    fn drop(&mut self) {
        self.shared.uses.fetch_sub(1, Ordering::SeqCst);
        self.shared.poll_waker.wake().unwrap();
    }
}

impl AsyncRead for Reader {
    fn poll_read(self: Pin<&mut Self>, cx: &mut Context<'_>, buf: &mut [u8]) -> Poll<io::Result<usize>> {
        self.shared.read_waker.register(cx.waker());
        match unsafe { self.shared.read(buf) } {
            Ok(n) => Poll::Ready(Ok(n)),
            Err(e) => match e.kind() {
                io::ErrorKind::WouldBlock => Poll::Pending,
                _ => Poll::Ready(Err(e)),
            },
        }
    }
}

impl AsyncWrite for Writer {
    fn poll_write(self: Pin<&mut Self>, cx: &mut Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        self.shared.write_waker.register(cx.waker());
        match unsafe { self.shared.write(buf) } {
            Ok(n) => {
                if n == buf.len() {
                    Poll::Ready(Ok(n))
                } else {
                    Poll::Ready(Err(io::ErrorKind::BrokenPipe.into()))
                }
            }
            Err(e) => match e.kind() {
                io::ErrorKind::WouldBlock => Poll::Pending,
                _ => Poll::Ready(Err(e)),
            },
        }
    }
    fn poll_flush(self: Pin<&mut Self>, _cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        Poll::Ready(Ok(()))
    }
    fn poll_close(self: Pin<&mut Self>, _cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        Poll::Ready(Ok(()))
    }
}
