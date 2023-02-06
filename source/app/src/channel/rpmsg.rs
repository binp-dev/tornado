use super::Channel;
use async_std::{
    fs::{File, OpenOptions},
    io::{Read, Write},
    os::unix::fs::OpenOptionsExt,
    path::Path,
};
use pin_project::pin_project;
use std::{
    io,
    os::fd::{AsRawFd, FromRawFd},
    pin::Pin,
    task::{Context, Poll},
};
use termios::Termios;

pub struct Rpmsg {
    file: File,
}

#[pin_project]
pub struct Reader {
    #[pin]
    file: File,
}

#[pin_project]
pub struct Writer {
    #[pin]
    file: File,
}

impl Rpmsg {
    pub async fn open<P: AsRef<Path>>(path: P) -> Result<Self, io::Error> {
        let file = OpenOptions::new()
            .read(true)
            .write(true)
            .custom_flags(libc::O_NOCTTY)
            .open(path)
            .await?;
        let fd = file.as_raw_fd();
        let mut tty = Termios::from_fd(fd)?;
        termios::cfmakeraw(&mut tty);
        termios::tcsetattr(fd, termios::TCSAFLUSH, &tty)?;
        Ok(Rpmsg { file })
    }
}

impl Channel for Rpmsg {
    type Read = Reader;
    type Write = Writer;
    fn split(self) -> (Reader, Writer) {
        let file = unsafe { File::from_raw_fd(self.file.as_raw_fd()) };
        (Reader { file }, Writer { file: self.file })
    }
}

impl Read for Reader {
    fn poll_read(self: Pin<&mut Self>, cx: &mut Context<'_>, buf: &mut [u8]) -> Poll<io::Result<usize>> {
        self.project().file.poll_read(cx, buf)
    }
}

impl Write for Writer {
    fn poll_write(self: Pin<&mut Self>, cx: &mut Context<'_>, buf: &[u8]) -> Poll<io::Result<usize>> {
        self.project().file.poll_write(cx, buf)
    }
    fn poll_flush(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        self.project().file.poll_flush(cx)
    }
    fn poll_close(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        self.project().file.poll_close(cx)
    }
}
