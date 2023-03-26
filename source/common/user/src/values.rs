use atomic_traits::Atomic;
use core::{
    fmt::Debug,
    sync::atomic::{AtomicI32, AtomicU16, AtomicU8},
};
use flatty::{
    mem::MaybeUninitUnsized, portable::le, prelude::NativeCast, Flat, FlatCheck, Portable,
};

pub trait Value: Copy + Send + Sync + 'static {
    type Base: Copy + Send + Sync + 'static + Default + Debug;
    const MIN: Self::Base;
    const MAX: Self::Base;
    fn try_from_base(base: Self::Base) -> Option<Self>;
    fn into_base(self) -> Self::Base;

    type Portable: Portable;
    fn from_portable(portable: Self::Portable) -> Self;
    fn into_portable(self) -> Self::Portable;

    type Atomic: Atomic<Type = Self::Base> + Default;
}

pub trait Analog: Value {
    const STEP: f64;

    fn into_analog(self) -> f64;

    fn try_from_analog(value: f64) -> Option<Self>;
    fn from_analog_saturating(value: f64) -> Self;
}

#[repr(transparent)]
#[derive(Clone, Copy, Debug, Eq, PartialEq, Ord, PartialOrd)]
pub struct DacPoint(pub u16);

impl DacPoint {
    pub const ZERO: u16 = 32767;
}
impl Default for DacPoint {
    fn default() -> Self {
        Self(DacPoint::ZERO)
    }
}
impl Value for DacPoint {
    type Base = u16;
    const MIN: u16 = u16::MIN;
    const MAX: u16 = u16::MAX;
    fn try_from_base(base: u16) -> Option<Self> {
        Some(DacPoint(base))
    }
    fn into_base(self) -> u16 {
        self.0
    }

    type Portable = le::U16;
    fn from_portable(portable: Self::Portable) -> Self {
        DacPoint(portable.to_native())
    }
    fn into_portable(self) -> Self::Portable {
        Self::Portable::from_native(self.0)
    }

    type Atomic = AtomicU16;
}
impl Analog for DacPoint {
    const STEP: f64 = 315.7445 * 1e-6;

    fn into_analog(self) -> f64 {
        (self.0 as i32 - Self::ZERO as i32) as f64 * Self::STEP
    }
    fn try_from_analog(value: f64) -> Option<Self> {
        let x = value / Self::STEP;
        if x >= Self::MIN as f64 - Self::ZERO as f64 && x <= Self::MAX as f64 - Self::ZERO as f64 {
            Some(Self(x as u16 + Self::ZERO))
        } else {
            None
        }
    }
    fn from_analog_saturating(value: f64) -> Self {
        let x = (value / Self::STEP).clamp(
            Self::MIN as f64 - Self::ZERO as f64,
            Self::MAX as f64 - Self::ZERO as f64,
        );
        Self((x as i32 + Self::ZERO as i32) as u16)
    }
}

#[repr(transparent)]
#[derive(Clone, Copy, Default, Debug, Eq, PartialEq, Ord, PartialOrd)]
pub struct AdcPoint(pub i32);
impl From<AdcPoint> for i32 {
    #[inline]
    fn from(value: AdcPoint) -> Self {
        value.0
    }
}
impl From<i32> for AdcPoint {
    #[inline]
    fn from(value: i32) -> Self {
        AdcPoint(value)
    }
}
impl Value for AdcPoint {
    type Base = i32;
    const MIN: i32 = i32::MIN;
    const MAX: i32 = i32::MAX;
    fn try_from_base(base: i32) -> Option<Self> {
        Some(AdcPoint(base))
    }
    fn into_base(self) -> i32 {
        self.0
    }

    type Portable = le::I32;
    fn from_portable(portable: Self::Portable) -> Self {
        AdcPoint(portable.to_native())
    }
    fn into_portable(self) -> Self::Portable {
        Self::Portable::from_native(self.0)
    }

    type Atomic = AtomicI32;
}
impl Analog for AdcPoint {
    const STEP: f64 = (346.8012 / 256.0) * 1e-6;

    fn into_analog(self) -> f64 {
        self.0 as f64 * AdcPoint::STEP
    }
    fn try_from_analog(value: f64) -> Option<Self> {
        let x = value / Self::STEP;
        if x >= Self::MIN as f64 && x <= Self::MAX as f64 {
            Some(Self(x as i32))
        } else {
            None
        }
    }
    fn from_analog_saturating(value: f64) -> Self {
        Self((value / Self::STEP).clamp(Self::MIN as f64, Self::MAX as f64) as i32)
    }
}

#[repr(transparent)]
#[derive(Clone, Copy, Default, Debug, Eq, PartialEq)]
pub struct Bits<const N: usize>(u8);
impl<const N: usize> Bits<N> {
    pub const SIZE: usize = N;
}
impl<const N: usize> From<Bits<N>> for u8 {
    fn from(value: Bits<N>) -> Self {
        value.0
    }
}
impl<const N: usize> TryFrom<u8> for Bits<N> {
    type Error = ();
    fn try_from(value: u8) -> Result<Self, Self::Error> {
        if value <= Self::MAX {
            Ok(Bits(value))
        } else {
            Err(())
        }
    }
}
impl<const N: usize> FlatCheck for Bits<N> {
    fn validate(this: &MaybeUninitUnsized<Self>) -> Result<&Self, flatty::Error> {
        if *unsafe { this.as_bytes().get_unchecked(0) } <= Self::MAX {
            Ok(unsafe { this.assume_init() })
        } else {
            Err(flatty::Error {
                kind: flatty::ErrorKind::InvalidData,
                pos: 0,
            })
        }
    }
}
unsafe impl<const N: usize> Flat for Bits<N> {}
unsafe impl<const N: usize> Portable for Bits<N> {}
impl<const N: usize> Value for Bits<N> {
    type Base = u8;
    const MIN: u8 = 0;
    const MAX: u8 = u8::MAX >> (8 - N);
    fn try_from_base(base: u8) -> Option<Self> {
        Self::try_from(base).ok()
    }
    fn into_base(self) -> u8 {
        self.into()
    }

    type Portable = Self;
    #[inline]
    fn from_portable(portable: Self::Portable) -> Self {
        portable
    }
    #[inline]
    fn into_portable(self) -> Self::Portable {
        self
    }

    type Atomic = AtomicU8;
}

pub type Din = Bits<8>;
pub type Dout = Bits<4>;
