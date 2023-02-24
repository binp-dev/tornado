use async_std::io::{Read, Write};
#[cfg(feature = "tcp")]
use async_std::{net::ToSocketAddrs, task::sleep};
#[cfg(feature = "tcp")]
use std::{io, time::Duration};

pub trait Channel: 'static {
    type Read: Read + Unpin + Send;
    type Write: Write + Unpin + Send;
    fn split(self) -> (Self::Read, Self::Write);
}

#[cfg(feature = "tcp")]
pub use async_std::net::TcpStream;
#[cfg(feature = "tcp")]
impl Channel for TcpStream {
    type Read = TcpStream;
    type Write = TcpStream;
    fn split(self) -> (Self::Read, Self::Write) {
        (self.clone(), self)
    }
}

#[cfg(feature = "tcp")]
pub async fn connect<A: ToSocketAddrs + Clone>(addr: A) -> Result<TcpStream, io::Error> {
    loop {
        match TcpStream::connect(addr.clone()).await {
            Ok(socket) => return Ok(socket),
            Err(err) => match err.kind() {
                io::ErrorKind::ConnectionRefused => {
                    sleep(Duration::from_millis(100)).await;
                }
                _ => return Err(err),
            },
        }
    }
}

#[cfg(feature = "rpmsg")]
mod rpmsg;
#[cfg(feature = "rpmsg")]
pub use rpmsg::Rpmsg;
