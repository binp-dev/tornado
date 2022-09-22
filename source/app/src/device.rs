use crate::{
    channel::Channel,
    config,
    epics::{self, Epics},
    proto::{AppMsg, McuMsg, McuMsgRef},
};
use async_std::sync::{Arc, Mutex};
use ferrite::channel::{MsgReader, MsgWriter};
use flatty::prelude::*;
use futures::{future::join_all, join};

struct Dac {
    channel: Arc<Mutex<MsgWriter<AppMsg, Channel>>>,
    epics: epics::Dac,
}

struct Adc {
    epics: epics::Adc,
}

pub struct Device {
    channel: MsgReader<McuMsg, Channel>,
    dacs: [Dac; config::DAC_COUNT],
    adcs: [Adc; config::ADC_COUNT],
}

impl Dac {
    fn new(channel: Arc<Mutex<MsgWriter<AppMsg, Channel>>>, epics: epics::Dac) -> Self {
        Self { channel, epics }
    }

    async fn run(self) {}
}

impl Adc {
    fn new(epics: epics::Adc) -> Self {
        Self { epics }
    }

    async fn push<I: Iterator<Item = config::Point>>(&mut self, _points: I) {}
}

impl Device {
    pub fn new(channel: Channel, epics: Epics) -> Self {
        let recv = MsgReader::<McuMsg, _>::new(channel.clone(), config::MAX_MCU_MSG_LEN);
        let send = Arc::new(Mutex::new(MsgWriter::<AppMsg, _>::new(channel, config::MAX_APP_MSG_LEN)));
        Self {
            channel: recv,
            dacs: epics.analog_outputs.map(|epx| Dac::new(send.clone(), epx)),
            adcs: epics.analog_inputs.map(Adc::new),
        }
    }

    pub async fn run(self) {
        let mut channel = self.channel;
        let mut adcs = self.adcs;
        join!(join_all(self.dacs.into_iter().map(|dac| dac.run())), async move {
            loop {
                match channel.read_msg().await.unwrap().as_ref() {
                    McuMsgRef::Empty(_) => (),
                    McuMsgRef::DinUpdate(_) => unimplemented!(),
                    McuMsgRef::DacRequest(_) => unimplemented!(),
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
        },);
    }
}
