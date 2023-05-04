use common::config::ADC_COUNT;
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
    pub addition: Variable<f64>,
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
    pub dac: Dac,
    pub adc: [Adc; ADC_COUNT],
    pub dout: Variable<u32>,
    pub din: Variable<u32>,
    pub debug: Debug,
}

impl Dac {
    fn new(reg: &mut Registry, index: usize) -> Result<Self, Error> {
        Ok(Self {
            scalar: reg.remove_downcast_suffix(&format!("ao{}", index))?,
            addition: reg.remove_downcast_suffix(&format!("ao{}:corr", index))?,
            array: reg.remove_downcast_suffix(&format!("aao{}", index))?,
            request: reg.remove_downcast_suffix(&format!("aao{}_request", index))?,
            mode: reg.remove_downcast_suffix(&format!("aao{}_mode", index))?,
            state: reg.remove_downcast_suffix(&format!("aao{}_state", index))?,
        })
    }
}

impl Adc {
    fn new(reg: &mut Registry, index: usize) -> Result<Self, Error> {
        Ok(Self {
            scalar: reg.remove_downcast_suffix(&format!("ai{}", index))?,
            array: reg.remove_downcast_suffix(&format!("aai{}", index))?,
        })
    }
}

impl Debug {
    fn new(reg: &mut Registry) -> Result<Self, Error> {
        Ok(Self {
            stats_reset: reg.remove_downcast_suffix("_stats_reset")?,
        })
    }
}

impl Epics {
    pub fn new(mut ctx: Context) -> Result<Self, Error> {
        let reg = &mut ctx.registry;
        let self_ = Self {
            dac: Dac::new(reg, 0)?,
            adc: (0..ADC_COUNT)
                .map(|i| Adc::new(reg, i))
                .collect::<Result<Vec<_>, _>>()?
                .try_into()
                .ok()
                .unwrap(),
            dout: reg.remove_downcast_suffix("do0")?,
            din: reg.remove_downcast_suffix("di0")?,
            debug: Debug::new(reg)?,
        };
        ctx.registry.check_empty()?;
        Ok(self_)
    }
}
