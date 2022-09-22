mod device;
mod epics;
mod proto;

use async_std::{net::TcpStream, sync::Mutex};
use ferrite::{
    channel::{MsgReader, MsgWriter},
    entry_point, Context,
};
use futures::{executor::block_on, join};
use macro_rules_attribute::apply;

use epics::Epics;
use flatty::portable::{le, NativeCast};
use proto::{AppMsg, AppMsgMut, AppMsgTag, McuMsg, McuMsgRef};

/// *Export symbols being called from IOC.*
pub use ferrite::export;

#[apply(entry_point)]
fn app_main(mut ctx: Context) {
    block_on(async_main(ctx));
}

async fn async_main(mut ctx: Context) {
    println!("[app]: IOC started");

    let epics = Epics::new(ctx).unwrap();

    let max_msg_size: usize = 496;
    let stream = TcpStream::connect("127.0.0.1:4884").await.unwrap();
    let mut reader = MsgReader::<McuMsg, _>::new(stream.clone(), max_msg_size);
    let writer = Mutex::new(MsgWriter::<AppMsg, _>::new(stream, max_msg_size));
    println!("[app]: Socket connected");

    join!(
        async {
            loop {
                let msg_guard = reader.read_msg().await.unwrap();
                match msg_guard.as_ref() {
                    McuMsgRef::Ai(msg) => {
                        println!("[app]: Msg.Ai");
                        ai.write(msg.value.to_native()).await;
                    }
                    McuMsgRef::Aai(msg) => {
                        println!("[app]: Msg.Aai");
                        assert!(msg.value.len() <= aai.max_len());
                        let mut aai_guard = aai.write_in_place().await;
                        for (src, dst) in msg.value.iter().zip(aai_guard.as_uninit_slice().iter_mut()) {
                            dst.write(src.to_native());
                        }
                        aai_guard.set_len(msg.value.len());
                    }
                }
            }
        },
        async {
            loop {
                let value = ao.read().await;
                println!("[app]: Ioc.Ao");
                let msg = proto::Ao {
                    value: le::I32::from_native(value),
                };
                let mut writer_guard = writer.lock().await;
                let mut msg_guard = writer_guard.init_default_msg().unwrap();
                msg_guard.reset_tag(AppMsgTag::Ao).unwrap();
                if let AppMsgMut::Ao(ao_msg) = msg_guard.as_mut() {
                    *ao_msg = msg;
                } else {
                    unreachable!();
                }
                msg_guard.write().await.unwrap();
            }
        },
        async {
            loop {
                let aao_guard = aao.read_in_place().await;
                println!("[app]: Ioc.Aao");
                let mut writer_guard = writer.lock().await;
                let mut msg_guard = writer_guard.init_default_msg().unwrap();
                msg_guard.reset_tag(AppMsgTag::Aao).unwrap();
                let data = if let AppMsgMut::Aao(msg) = msg_guard.as_mut() {
                    &mut msg.value
                } else {
                    unreachable!();
                };
                for value in aao_guard.as_slice() {
                    data.push(le::I32::from_native(*value)).unwrap();
                }
                msg_guard.write().await.unwrap();
            }
        }
    );
}
