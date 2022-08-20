use std::{
    io::{self, prelude::*},
    net::TcpStream,
    str::from_utf8,
};

fn main() -> io::Result<()> {
    let mut stream = TcpStream::connect("127.0.0.1:4884")?;

    let send_msg = b"Hello, Fakedev!";
    stream.write_all(send_msg)?;
    println!("A -> F: {}", from_utf8(send_msg).unwrap());

    let recv_msg = b"Hi, App!";
    let mut recv_buf = vec![0; recv_msg.len()];
    stream.read_exact(&mut recv_buf)?;
    println!("A <- F: {}", from_utf8(&recv_buf).unwrap());
    assert_eq!(recv_buf.as_slice(), recv_msg);

    Ok(())
}
