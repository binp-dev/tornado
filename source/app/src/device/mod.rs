mod adc;
mod dac;

use crate::{
    channel::Channel,
    config,
    epics::Epics,
    proto::{AppMsg, McuMsg, McuMsgRef},
};
use async_std::sync::{Arc, Mutex};
use ferrite::channel::{MsgReader, MsgWriter};
use flatty::prelude::*;
use futures::{future::join_all, join};

use adc::{Adc, AdcHandle};
use dac::{Dac, DacHandle};

pub struct Device {
    channel: MsgReader<McuMsg, Channel>,
    dacs: [Dac; config::DAC_COUNT],
    adcs: [Adc; config::ADC_COUNT],
}

pub struct MsgDispatcher {
    channel: MsgReader<McuMsg, Channel>,
    dacs: [DacHandle; config::DAC_COUNT],
    adcs: [AdcHandle; config::ADC_COUNT],
}

impl Device {
    pub fn new(channel: Channel, epics: Epics) -> Self {
        let recv = MsgReader::<McuMsg, _>::new(channel.clone(), config::MAX_MCU_MSG_LEN);
        let send = Arc::new(Mutex::new(MsgWriter::<AppMsg, _>::new(channel, config::MAX_APP_MSG_LEN)));
        Self {
            channel: recv,
            dacs: epics.analog_outputs.map(|epics| Dac {
                channel: send.clone(),
                epics,
            }),
            adcs: epics.analog_inputs.map(|epics| Adc { epics }),
        }
    }

    pub async fn run(self) {
        let (dac_loops, dac_handles): (Vec<_>, Vec<_>) = self.dacs.into_iter().map(|dac| dac.run()).unzip();
        let (adc_loops, adc_handles): (Vec<_>, Vec<_>) = self.adcs.into_iter().map(|adc| adc.run()).unzip();
        let dispatcher = MsgDispatcher {
            channel: self.channel,
            dacs: dac_handles.try_into().ok().unwrap(),
            adcs: adc_handles.try_into().ok().unwrap(),
        };
        join!(join_all(dac_loops), join_all(adc_loops), dispatcher.run());
    }
}

impl MsgDispatcher {
    pub async fn run(self) {
        let mut channel = self.channel;
        let mut adcs = self.adcs;
        loop {
            match channel.read_msg().await.unwrap().as_ref() {
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
