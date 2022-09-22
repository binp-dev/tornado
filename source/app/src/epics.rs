use ferrite::{AnyVariable, Context, Downcast, ReadArrayVariable, ReadVariable, Registry, WriteArrayVariable, WriteVariable};

#[derive(Clone, Debug)]
pub enum EpicsError {
    NoSuchPv(String),
    WrongPvType,
    UnusedPvs,
}

pub const DAC_COUNT: usize = 1;
pub const ADC_COUNT: usize = 6;

pub struct Dac {
    pub scalar: ReadVariable<i32>,
    pub array: ReadArrayVariable<f64>,
    pub request: WriteVariable<bool>,
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
    pub stats_reset: ReadVariable<bool>,
}

fn try_downcast<R>(registry: &mut Registry, name: &str) -> Result<R, EpicsError>
where
    AnyVariable: Downcast<R>,
{
    registry
        .remove(name)
        .ok_or_else(|| EpicsError::NoSuchPv(String::from(name)))?
        .downcast()
        .ok_or_else(|| EpicsError::WrongPvType)
}

impl Dac {
    fn new(reg: &mut Registry, name: &str) -> Result<Self, EpicsError> {
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
    fn new(reg: &mut Registry, name: &str) -> Result<Self, EpicsError> {
        Ok(Self {
            scalar: try_downcast(reg, name)?,
            array: try_downcast(reg, &format!("a{}", name))?,
        })
    }
}

impl Epics {
    pub fn new(mut ctx: Context) -> Result<Self, EpicsError> {
        let mut reg = &mut ctx.registry;
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
            return Err(EpicsError::UnusedPvs);
        }
        Ok(self_)
    }
}
