mod proto;

use async_std::net::TcpStream;
use ferrite::{
    channel::{MsgReader, MsgWriter},
    entry_point, AnyVariable, Context, Downcast, ReadArrayVariable, ReadVariable, Registry, WriteArrayVariable, WriteVariable,
};
use futures::{executor::block_on, join};
use macro_rules_attribute::apply;

use flatty::portable::{le, NativeCast};
use proto::{InMsg, InMsgRef, OutMsg, OutMsgMut, OutMsgTag};

/// *Export symbols being called from IOC.*
pub use ferrite::export;

#[apply(entry_point)]
fn app_main(mut ctx: Context) {
    block_on(async_main(ctx));
}

fn take_from_registry<V>(reg: &mut Registry, name: &str) -> Option<V>
where
    AnyVariable: Downcast<V>,
{
    reg.remove(name)?.downcast()
}

async fn async_main(mut ctx: Context) {
    use env_logger::Env;
    env_logger::Builder::from_env(Env::default().default_filter_or("info")).init();

    log::info!("IOC started");

    let mut ai: WriteVariable<i32> = take_from_registry(&mut ctx.registry, "ai").unwrap();
    let mut ao: ReadVariable<i32> = take_from_registry(&mut ctx.registry, "ao").unwrap();
    let mut aai: WriteArrayVariable<i32> = take_from_registry(&mut ctx.registry, "aai").unwrap();
    let mut aao: ReadArrayVariable<i32> = take_from_registry(&mut ctx.registry, "aao").unwrap();
    let mut waveform: WriteArrayVariable<i32> = take_from_registry(&mut ctx.registry, "waveform").unwrap();
    let mut bi: WriteVariable<u32> = take_from_registry(&mut ctx.registry, "bi").unwrap();
    let mut bo: ReadVariable<u32> = take_from_registry(&mut ctx.registry, "bo").unwrap();
    let mut mbbi_direct: WriteVariable<u32> = take_from_registry(&mut ctx.registry, "mbbiDirect").unwrap();
    let mut mbbo_direct: ReadVariable<u32> = take_from_registry(&mut ctx.registry, "mbboDirect").unwrap();

    assert!(ctx.registry.is_empty());

    let max_msg_size: usize = 259;
    let stream = TcpStream::connect("127.0.0.1:4884").await.unwrap();
    let mut reader = MsgReader::<InMsg, _>::new(stream.clone(), max_msg_size);
    let writer = MsgWriter::<OutMsg, _>::new(stream, max_msg_size);
    log::info!("Socket connected");

    join!(
        async {
            loop {
                let msg = reader.read_msg().await.unwrap();
                match msg.as_ref() {
                    InMsgRef::Ai(msg) => {
                        log::debug!("Msg.Ai");
                        ai.write(msg.value.to_native()).await;
                    }
                    InMsgRef::Aai(msg) => {
                        log::debug!("Msg.Aai");
                        assert!(msg.values.len() <= aai.max_len());
                        let mut var = aai.write_in_place().await;
                        for (src, dst) in msg.values.iter().zip(var.as_uninit_slice().iter_mut()) {
                            dst.write(src.to_native());
                        }
                        var.set_len(msg.values.len());
                    }
                    InMsgRef::Waveform(msg) => {
                        log::debug!("Msg.Waveform");
                        assert!(msg.values.len() <= waveform.max_len());
                        let mut var = waveform.write_in_place().await;
                        for (src, dst) in msg.values.iter().zip(var.as_uninit_slice().iter_mut()) {
                            dst.write(src.to_native());
                        }
                        var.set_len(msg.values.len());
                    }
                    InMsgRef::Bi(msg) => {
                        log::debug!("Msg.Bi");
                        bi.write(msg.value.to_native()).await;
                    }
                    InMsgRef::MbbiDirect(msg) => {
                        log::debug!("Msg.MbbiDirect");
                        mbbi_direct.write(msg.value.to_native()).await;
                    }
                }
            }
        },
        async {
            let mut writer = writer.clone();
            loop {
                let value = ao.read().await;
                log::debug!("Ioc.Ao");
                let mut msg = writer.init_default_msg().unwrap();
                msg.reset_tag(OutMsgTag::Ao).unwrap();
                if let OutMsgMut::Ao(msg) = msg.as_mut() {
                    *msg = proto::Ao {
                        value: le::I32::from_native(value),
                    };
                } else {
                    unreachable!();
                }
                msg.write().await.unwrap();
            }
        },
        async {
            let mut writer = writer.clone();
            loop {
                let var = aao.read_in_place().await;
                log::debug!("Ioc.Aao");
                let mut msg = writer.init_default_msg().unwrap();
                msg.reset_tag(OutMsgTag::Aao).unwrap();
                let data = if let OutMsgMut::Aao(msg) = msg.as_mut() {
                    &mut msg.values
                } else {
                    unreachable!();
                };
                for value in var.as_slice() {
                    data.push(le::I32::from_native(*value)).unwrap();
                }
                msg.write().await.unwrap();
            }
        },
        async {
            let mut writer = writer.clone();
            loop {
                let value = bo.read().await;
                log::debug!("Ioc.Bo");
                let mut msg = writer.init_default_msg().unwrap();
                msg.reset_tag(OutMsgTag::Bo).unwrap();
                if let OutMsgMut::Bo(msg) = msg.as_mut() {
                    *msg = proto::Bo {
                        value: le::U32::from_native(value),
                    };
                } else {
                    unreachable!();
                }
                msg.write().await.unwrap();
            }
        },
        async {
            let mut writer = writer.clone();
            loop {
                let value = mbbo_direct.read().await;
                log::debug!("Ioc.MbboDirect");
                let mut msg = writer.init_default_msg().unwrap();
                msg.reset_tag(OutMsgTag::MbboDirect).unwrap();
                if let OutMsgMut::MbboDirect(msg) = msg.as_mut() {
                    *msg = proto::MbboDirect {
                        value: le::U32::from_native(value),
                    };
                } else {
                    unreachable!();
                }
                msg.write().await.unwrap();
            }
        },
    );
}
