use crate::epics::{self, Epics};
use async_std::{net::TcpStream, sync::Mutex};

struct Dac {}

struct Adc {}

pub struct Device {
    channel: Mutex<TcpStream>,
    dacs: [Dac; epics::DAC_COUNT],
    adcs: [Adc; epics::ADC_COUNT],
}

impl Dac {
    fn new(epics: epics::Dac) -> Self {
        Self {}
    }

    async fn run(self) {}
}

impl Adc {
    fn new(epics: epics::Adc) -> Self {
        Self {}
    }
}

impl Device {
    pub fn new(epics: Epics, channel: TcpStream) -> Self {
        Self {
            channel: Mutex::new(channel),
            dacs: epics.analog_outputs.map(|epx| Dac::new(epx)),
            adcs: epics.analog_inputs.map(|epx| Adc::new(epx)),
        }
    }

    async fn run(self) {}
}
