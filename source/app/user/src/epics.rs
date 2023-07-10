use common::config::AI_COUNT;
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

pub struct Ao {
    pub next_waveform: Variable<[f64]>,
    pub add: Variable<f64>,
    pub next_cycle: Variable<u16>,
    pub next_ready: Variable<u16>,
}

pub struct Ai {
    pub waveform: Variable<[f64]>,
}

pub struct Debug {
    pub reset_stats: Variable<u16>,
}

/// EPICS interface
pub struct Epics {
    pub ao: Ao,
    pub ais: [Ai; AI_COUNT],
    pub do_: Variable<u32>,
    pub di: Variable<u32>,
    pub debug: Debug,
}

impl Ao {
    fn new(reg: &mut Registry) -> Result<Self, Error> {
        Ok(Self {
            next_waveform: reg.remove_downcast_suffix("Ao0Next")?,
            add: reg.remove_downcast_suffix("Ao0Add")?,
            next_ready: reg.remove_downcast_suffix("AoNextReady")?,
            next_cycle: reg.remove_downcast_suffix("AoNextCycle")?,
        })
    }
}

impl Ai {
    fn new(reg: &mut Registry, index: usize) -> Result<Self, Error> {
        Ok(Self {
            waveform: reg.remove_downcast_suffix(&format!("Ai{}", index))?,
        })
    }
}

impl Debug {
    fn new(reg: &mut Registry) -> Result<Self, Error> {
        Ok(Self {
            reset_stats: reg.remove_downcast_suffix("DebugResetStats")?,
        })
    }
}

impl Epics {
    pub fn new(mut ctx: Context) -> Result<Self, Error> {
        let reg = &mut ctx.registry;
        let mut ais = Vec::new();
        for index in 0..AI_COUNT {
            ais.push(Ai::new(reg, index)?);
        }
        let self_ = Self {
            ao: Ao::new(reg)?,
            ais: ais.try_into().ok().unwrap(),
            do_: reg.remove_downcast_suffix("Do")?,
            di: reg.remove_downcast_suffix("Di")?,
            debug: Debug::new(reg)?,
        };
        ctx.registry.check_empty()?;
        Ok(self_)
    }
}
