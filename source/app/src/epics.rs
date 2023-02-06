use crate::config::{ADC_COUNT, DAC_COUNT};
use ferrite::{
    variable::registry::{CheckEmptyError, GetDowncastError},
    ArrayVariable, Context, Registry, Variable,
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
    pub array: ArrayVariable<f64>,
    pub request: Variable<u16>,
    pub mode: Variable<u16>,
    pub state: Variable<u16>,
}

pub struct Adc {
    pub scalar: Variable<f64>,
    pub array: ArrayVariable<f64>,
}

/// EPICS interface
pub struct Epics {
    pub analog_outputs: [Dac; DAC_COUNT],
    pub analog_inputs: [Adc; ADC_COUNT],
    pub discrete_output: Variable<u32>,
    pub discrete_input: Variable<u32>,
    pub stats_reset: Variable<u16>,
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

impl Epics {
    pub fn new(mut ctx: Context) -> Result<Self, Error> {
        let reg = &mut ctx.registry;
        let self_ = Self {
            analog_outputs: (0..DAC_COUNT)
                .map(|i| Dac::new(reg, &format!("ao{}", i)))
                .collect::<Result<Vec<_>, _>>()?
                .try_into()
                .ok()
                .unwrap(),
            analog_inputs: (0..ADC_COUNT)
                .map(|i| Adc::new(reg, &format!("ai{}", i)))
                .collect::<Result<Vec<_>, _>>()?
                .try_into()
                .ok()
                .unwrap(),
            discrete_output: reg.remove_downcast("do0")?,
            discrete_input: reg.remove_downcast("di0")?,
            stats_reset: reg.remove_downcast("stats_reset")?,
        };
        ctx.registry.check_empty()?;
        Ok(self_)
    }
}
