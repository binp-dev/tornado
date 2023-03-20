#[cfg(feature = "tcp")]
use async_std::net::TcpStream;

#[cfg(feature = "tcp")]
pub type Channel = TcpStream;
