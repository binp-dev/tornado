#[cfg(feature = "tcp")]
use std::{io, time::Duration};
use tokio::io::{AsyncRead as Read, AsyncWrite as Write};
#[cfg(feature = "tcp")]
use tokio::{net::ToSocketAddrs, time::sleep};

pub trait Channel: 'static {
    type Read: Read + Unpin + Send;
    type Write: Write + Unpin + Send;
    fn split(self) -> (Self::Read, Self::Write);
}

#[cfg(feature = "tcp")]
pub use tokio::net::{
    tcp::{OwnedReadHalf, OwnedWriteHalf},
    TcpStream,
};
#[cfg(feature = "tcp")]
impl Channel for TcpStream {
    type Read = OwnedReadHalf;
    type Write = OwnedWriteHalf;
    fn split(self) -> (Self::Read, Self::Write) {
        self.into_split()
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
pub mod rpmsg;
#[cfg(feature = "rpmsg")]
pub use rpmsg::Rpmsg;
