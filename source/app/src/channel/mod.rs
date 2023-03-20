use async_std::io::{Read, Write};

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

#[cfg(feature = "rpmsg")]
mod rpmsg;
#[cfg(feature = "rpmsg")]
pub use rpmsg::Rpmsg;
