mod adc;
mod dac;

use crate::{
    channel::Channel,
    config,
    epics::Epics,
    proto::{AppMsg, AppMsgTag, McuMsg, McuMsgRef},
};
use async_std::{
    sync::{Arc, Mutex},
    task::sleep,
};
use ferrite::channel::{MsgReader, MsgWriter};
use flatty::prelude::*;
use futures::{future::join_all, join};

use adc::{Adc, AdcHandle};
use dac::{Dac, DacHandle};

pub struct Device {
    reader: MsgReader<McuMsg, Channel>,
    writer: Arc<Mutex<MsgWriter<AppMsg, Channel>>>,
    dacs: [Dac; config::DAC_COUNT],
    adcs: [Adc; config::ADC_COUNT],
}

struct MsgDispatcher {
    channel: MsgReader<McuMsg, Channel>,
    dacs: [DacHandle; config::DAC_COUNT],
    adcs: [AdcHandle; config::ADC_COUNT],
}

struct Keepalive {
    channel: Arc<Mutex<MsgWriter<AppMsg, Channel>>>,
}

impl Device {
    pub fn new(channel: Channel, epics: Epics) -> Self {
        let reader = MsgReader::<McuMsg, _>::new(channel.clone(), config::MAX_MCU_MSG_LEN);
        let writer = Arc::new(Mutex::new(MsgWriter::<AppMsg, _>::new(channel, config::MAX_APP_MSG_LEN)));
        Self {
            dacs: epics.analog_outputs.map(|epics| Dac {
                channel: writer.clone(),
                epics,
            }),
            adcs: epics.analog_inputs.map(|epics| Adc { epics }),
            reader,
            writer,
        }
    }

    pub async fn run(self) {
        let (dac_loops, dac_handles): (Vec<_>, Vec<_>) = self.dacs.into_iter().map(|dac| dac.run()).unzip();
        let (adc_loops, adc_handles): (Vec<_>, Vec<_>) = self.adcs.into_iter().map(|adc| adc.run()).unzip();
        let dispatcher = MsgDispatcher {
            channel: self.reader,
            dacs: dac_handles.try_into().ok().unwrap(),
            adcs: adc_handles.try_into().ok().unwrap(),
        };
        let keepalive = Keepalive { channel: self.writer };
        join!(join_all(dac_loops), join_all(adc_loops), dispatcher.run(), keepalive.run());
    }
}

impl MsgDispatcher {
    async fn run(self) {
        let mut channel = self.channel;
        let mut adcs = self.adcs;
        loop {
            let msg = channel.read_msg().await.unwrap();
            println!("read_msg: {:?}", msg.tag());
            match msg.as_ref() {
                McuMsgRef::Empty(_) => (),
                McuMsgRef::DinUpdate(_) => unimplemented!(),
                McuMsgRef::DacRequest(req) => self.dacs[0].request(req.count.to_native() as usize),
                McuMsgRef::AdcData(data) => {
                    for (index, adc) in adcs.iter_mut().enumerate() {
                        adc.push(data.points_arrays.iter().map(|a| a[index].to_native())).await
                    }
                }
                McuMsgRef::Error(error) => {
                    panic!("Error {}: {}", error.code, String::from_utf8_lossy(error.message.as_slice()))
                }
                McuMsgRef::Debug(debug) => {
                    println!("Debug: {}", String::from_utf8_lossy(debug.message.as_slice()))
                }
            }
        }
    }
}

impl Keepalive {
    async fn run(self) {
        {
            let mut channel_guard = self.channel.lock().await;
            let mut msg_guard = channel_guard.init_default_msg().unwrap();
            msg_guard.reset_tag(AppMsgTag::Connect).unwrap();
            msg_guard.write().await.unwrap();
        }
        loop {
            sleep(config::KEEP_ALIVE_PERIOD).await;
            println!("keepalive");
            let mut channel_guard = self.channel.lock().await;
            println!("lock channel (keepalive)");
            let mut msg_guard = channel_guard.init_default_msg().unwrap();
            msg_guard.reset_tag(AppMsgTag::KeepAlive).unwrap();
            println!("write msg (keepalive)");
            msg_guard.write().await.unwrap();
        }
    }
}
