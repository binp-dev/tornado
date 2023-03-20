use crate::config::{ADC_COUNT, DAC_COUNT};
use ferrite::{
    AnyVariable, Context, Downcast, ReadArrayVariable, ReadVariable, Registry, VariableType, WriteArrayVariable, WriteVariable,
};

#[derive(Clone, Debug)]
pub enum Error {
    NoSuchPv(String),
    WrongPvType(String, VariableType),
    UnusedPvs(Vec<String>),
}

pub struct Dac {
    pub scalar: ReadVariable<i32>,
    pub array: ReadArrayVariable<f64>,
    pub request: WriteVariable<u32>,
    pub mode: ReadVariable<u32>,
    pub state: ReadVariable<u32>,
}

pub struct Adc {
    pub scalar: WriteVariable<i32>,
    pub array: WriteArrayVariable<f64>,
}

/// EPICS interface
pub struct Epics {
    pub analog_outputs: [Dac; DAC_COUNT],
    pub analog_inputs: [Adc; ADC_COUNT],
    pub discrete_output: ReadVariable<u32>,
    pub discrete_input: WriteVariable<u32>,
    pub stats_reset: ReadVariable<u32>,
}

fn try_downcast<R>(registry: &mut Registry, name: &str) -> Result<R, Error>
where
    AnyVariable: Downcast<R>,
{
    let any = registry.remove(name).ok_or_else(|| Error::NoSuchPv(String::from(name)))?;
    let data_type = any.data_type();
    any.downcast()
        .ok_or_else(|| Error::WrongPvType(String::from(name), data_type))
}

impl Dac {
    fn new(reg: &mut Registry, name: &str) -> Result<Self, Error> {
        Ok(Self {
            scalar: try_downcast(reg, name)?,
            array: try_downcast(reg, &format!("a{}", name))?,
            request: try_downcast(reg, &format!("a{}_request", name))?,
            mode: try_downcast(reg, &format!("a{}_mode", name))?,
            state: try_downcast(reg, &format!("a{}_state", name))?,
        })
    }
}

impl Adc {
    fn new(reg: &mut Registry, name: &str) -> Result<Self, Error> {
        Ok(Self {
            scalar: try_downcast(reg, name)?,
            array: try_downcast(reg, &format!("a{}", name))?,
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
            discrete_output: try_downcast(reg, "do0")?,
            discrete_input: try_downcast(reg, "di0")?,
            stats_reset: try_downcast(reg, "stats_reset")?,
        };
        if !ctx.registry.is_empty() {
            return Err(Error::UnusedPvs(ctx.registry.keys().map(String::from).collect()));
        }
        Ok(self_)
    }
}
