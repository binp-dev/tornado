use atomic_traits::{fetch, Atomic, NumOps};
use core::{
    fmt::{Debug, Display, LowerHex},
    sync::atomic::{AtomicI32, AtomicU16},
};
use derive_more::{Display, From, Into};
use flatty::{portable::le, prelude::NativeCast, Portable};

pub trait Unit:
    Copy + Send + Sync + 'static + Default + From<Self::Base> + Into<Self::Base> + Debug + Display
{
    type Base: Copy + Send + Sync + 'static + Into<i64> + TryFrom<i64> + Debug + Display + LowerHex;

    const MIN: Self;
    const MAX: Self;
    const ZERO: Self;

    type Atomic: Atomic<Type = Self::Base> + NumOps + fetch::Min + fetch::Max + Default;

    type Portable: Portable;

    fn to_portable(self) -> Self::Portable;
    fn from_portable(p: Self::Portable) -> Self;
}

pub trait Voltage: Unit {
    const STEP: f64;

    fn to_voltage(self) -> f64;
    fn try_from_voltage(v: f64) -> Option<Self>;
    fn from_voltage_saturating(v: f64) -> Self;
}

#[repr(transparent)]
#[derive(Clone, Copy, Debug, Eq, PartialEq, Ord, PartialOrd, From, Into, Display)]
pub struct DacPoint(pub u16);

impl Default for DacPoint {
    fn default() -> Self {
        DacPoint::ZERO
    }
}

impl Voltage for DacPoint {
    const STEP: f64 = 315.7445 * 1e-6;

    fn to_voltage(self) -> f64 {
        (self.0 as i32 - Self::ZERO.0 as i32) as f64 * Self::STEP
    }
    fn try_from_voltage(v: f64) -> Option<Self> {
        let x = v / Self::STEP;
        if x >= Self::MIN.0 as f64 - Self::ZERO.0 as f64
            && x <= Self::MAX.0 as f64 - Self::ZERO.0 as f64
        {
            Some(Self(x as u16 + Self::ZERO.0))
        } else {
            None
        }
    }
    fn from_voltage_saturating(v: f64) -> Self {
        let x = (v / Self::STEP).clamp(
            Self::MIN.0 as f64 - Self::ZERO.0 as f64,
            Self::MAX.0 as f64 - Self::ZERO.0 as f64,
        );
        Self((x as i32 + Self::ZERO.0 as i32) as u16)
    }
}

impl Unit for DacPoint {
    type Base = u16;

    const MIN: DacPoint = DacPoint(u16::MIN);
    const MAX: DacPoint = DacPoint(u16::MAX);
    const ZERO: DacPoint = DacPoint(32767);

    type Atomic = AtomicU16;
    type Portable = le::U16;

    fn to_portable(self) -> Self::Portable {
        Self::Portable::from_native(self.0)
    }
    fn from_portable(portable: Self::Portable) -> Self {
        Self(portable.to_native())
    }
}

impl From<DacPoint> for f64 {
    fn from(value: DacPoint) -> Self {
        value.to_voltage()
    }
}
impl TryFrom<f64> for DacPoint {
    type Error = ();
    fn try_from(value: f64) -> Result<Self, Self::Error> {
        Self::try_from_voltage(value).ok_or(())
    }
}

#[repr(transparent)]
#[derive(Clone, Copy, Default, Debug, Eq, PartialEq, Ord, PartialOrd, From, Into, Display)]
pub struct AdcPoint(pub i32);

impl Voltage for AdcPoint {
    const STEP: f64 = (346.8012 / 256.0) * 1e-6;

    fn to_voltage(self) -> f64 {
        self.0 as f64 * Self::STEP
    }
    fn try_from_voltage(v: f64) -> Option<Self> {
        let x = v / Self::STEP;
        if x >= Self::MIN.0 as f64 && x <= Self::MAX.0 as f64 {
            Some(Self(x as i32))
        } else {
            None
        }
    }
    fn from_voltage_saturating(v: f64) -> Self {
        Self((v / Self::STEP).clamp(Self::MIN.0 as f64, Self::MAX.0 as f64) as i32)
    }
}

impl Unit for AdcPoint {
    type Base = i32;

    const MIN: AdcPoint = AdcPoint(i32::MIN);
    const MAX: AdcPoint = AdcPoint(i32::MAX);
    const ZERO: AdcPoint = AdcPoint(0);

    type Atomic = AtomicI32;
    type Portable = le::I32;

    fn to_portable(self) -> Self::Portable {
        Self::Portable::from_native(self.0)
    }
    fn from_portable(portable: Self::Portable) -> Self {
        Self(portable.to_native())
    }
}
