use common::config::{ADC_COUNT, DAC_COUNT};
use ferrite::{
    registry::{CheckEmptyError, GetDowncastError},
    Context, Registry, TypedVariable as Variable,
};
use thiserror::Error;

#[derive(Clone, Debug, Error)]
pub enum Error {
    #[error("{0}")]
    Take(#[from] GetDowncastError),
    #[error("{0}")]
    Unused(#[from] CheckEmptyError),
}

pub struct Dac {
    pub scalar: Variable<f64>,
    pub array: Variable<[f64]>,
    pub request: Variable<u16>,
    pub mode: Variable<u16>,
    pub state: Variable<u16>,
}

pub struct Adc {
    pub scalar: Variable<f64>,
    pub array: Variable<[f64]>,
}

pub struct Debug {
    pub stats_reset: Variable<u16>,
}

/// EPICS interface
pub struct Epics {
    pub dac: [Dac; DAC_COUNT],
    pub adc: [Adc; ADC_COUNT],
    pub dout: Variable<u32>,
    pub din: Variable<u32>,
    pub debug: Debug,
}

impl Dac {
    fn new(reg: &mut Registry, name: &str) -> Result<Self, Error> {
        Ok(Self {
            scalar: reg.remove_downcast(name)?,
            array: reg.remove_downcast(&format!("a{}", name))?,
            request: reg.remove_downcast(&format!("a{}_request", name))?,
            mode: reg.remove_downcast(&format!("a{}_mode", name))?,
            state: reg.remove_downcast(&format!("a{}_state", name))?,
        })
    }
}

impl Adc {
    fn new(reg: &mut Registry, name: &str) -> Result<Self, Error> {
        Ok(Self {
            scalar: reg.remove_downcast(name)?,
            array: reg.remove_downcast(&format!("a{}", name))?,
        })
    }
}

impl Debug {
    fn new(reg: &mut Registry) -> Result<Self, Error> {
        Ok(Self {
            stats_reset: reg.remove_downcast("stats_reset")?,
        })
    }
}

impl Epics {
    pub fn new(mut ctx: Context) -> Result<Self, Error> {
        let reg = &mut ctx.registry;
        let self_ = Self {
            dac: (0..DAC_COUNT)
                .map(|i| Dac::new(reg, &format!("ao{}", i)))
                .collect::<Result<Vec<_>, _>>()?
                .try_into()
                .ok()
                .unwrap(),
            adc: (0..ADC_COUNT)
                .map(|i| Adc::new(reg, &format!("ai{}", i)))
                .collect::<Result<Vec<_>, _>>()?
                .try_into()
                .ok()
                .unwrap(),
            dout: reg.remove_downcast("do0")?,
            din: reg.remove_downcast("di0")?,
            debug: Debug::new(reg)?,
        };
        ctx.registry.check_empty()?;
        Ok(self_)
    }
}
