use super::Channel;
use futures::ready;
use std::{
    fs::{File, OpenOptions},
    io,
    os::{
        fd::{AsRawFd, FromRawFd, IntoRawFd, RawFd},
        raw::c_void,
        unix::fs::OpenOptionsExt,
    },
    path::Path,
    pin::Pin,
    sync::Arc,
    task::{Context, Poll},
};
use termios::Termios;
use tokio::io::{unix::AsyncFd, AsyncRead, AsyncWrite, ReadBuf};

pub struct Rpmsg {
    fd: RawFd,
}

impl AsRawFd for Rpmsg {
    fn as_raw_fd(&self) -> RawFd {
        self.fd
    }
}

pub struct Reader {
    raw: Arc<AsyncFd<Rpmsg>>,
}

pub struct Writer {
    raw: Arc<AsyncFd<Rpmsg>>,
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

        Ok(Rpmsg { fd })
    }
}

impl Channel for Rpmsg {
    type Read = Reader;
    type Write = Writer;

    fn split(self) -> (Reader, Writer) {
        let raw = Arc::new(AsyncFd::new(self).unwrap());
        (Reader { raw: raw.clone() }, Writer { raw })
    }
}

impl Rpmsg {
    fn read(&self, dst: &mut [u8]) -> io::Result<usize> {
        let r = unsafe { libc::read(self.as_raw_fd(), dst.as_ptr() as *mut c_void, dst.len()) };
        if r < 0 {
            Err(io::Error::last_os_error())
        } else {
            Ok(r as usize)
        }
    }

    fn write(&self, src: &[u8]) -> io::Result<usize> {
        let r = unsafe { libc::write(self.as_raw_fd(), src.as_ptr() as *const c_void, src.len()) };
        if r < 0 {
            Err(io::Error::last_os_error())
        } else {
            Ok(r as usize)
        }
    }
}

impl Drop for Rpmsg {
    fn drop(&mut self) {
        unsafe { File::from_raw_fd(self.fd) };
    }
}

impl AsyncRead for Reader {
    fn poll_read(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<io::Result<()>> {
        loop {
            let mut guard = ready!(self.raw.poll_read_ready(cx))?;
            let dst = buf.initialize_unfilled();
            match guard.try_io(|raw| raw.get_ref().read(dst)) {
                Ok(Ok(n)) => {
                    buf.advance(n);
                    break Poll::Ready(Ok(()));
                }
                Ok(Err(e)) => break Poll::Ready(Err(e)),
                Err(_) => continue,
            }
        }
    }
}

impl AsyncWrite for Writer {
    fn poll_write(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &[u8],
    ) -> Poll<io::Result<usize>> {
        loop {
            let mut guard = ready!(self.raw.poll_write_ready(cx))?;
            match guard.try_io(|raw| raw.get_ref().write(buf)) {
                Ok(r) => break Poll::Ready(r),
                Err(_) => continue,
            }
        }
    }
    fn poll_flush(self: Pin<&mut Self>, _cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        Poll::Ready(Ok(()))
    }
    fn poll_shutdown(self: Pin<&mut Self>, _cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        Poll::Ready(Ok(()))
    }
}
