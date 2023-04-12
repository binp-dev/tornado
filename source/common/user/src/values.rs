use crate::config::{DIN_BITS, DOUT_BITS};
use atomic_traits::Atomic;
use core::{
    fmt::Debug,
    sync::atomic::{AtomicI32, AtomicU8},
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

#[repr(transparent)]
#[derive(Clone, Copy, Default, Debug, Eq, PartialEq, Ord, PartialOrd)]
pub struct Point(pub i32);
impl From<Point> for i32 {
    #[inline]
    fn from(value: Point) -> Self {
        value.0
    }
}
impl From<i32> for Point {
    #[inline]
    fn from(value: i32) -> Self {
        Point(value)
    }
}
impl Value for Point {
    type Base = i32;
    const MIN: i32 = i32::MIN;
    const MAX: i32 = i32::MAX;
    fn try_from_base(base: i32) -> Option<Self> {
        Some(Point(base))
    }
    fn into_base(self) -> i32 {
        self.0
    }

    type Portable = le::I32;
    fn from_portable(portable: Self::Portable) -> Self {
        Point(portable.to_native())
    }
    fn into_portable(self) -> Self::Portable {
        Self::Portable::from_native(self.0)
    }

    type Atomic = AtomicI32;
}
impl Point {
    pub const STEP: f64 = 1e-6;

    pub fn into_analog(self) -> f64 {
        self.0 as f64 * Point::STEP
    }
    pub fn try_from_analog(value: f64) -> Option<Self> {
        let x = value / Self::STEP;
        if x >= Self::MIN as f64 && x <= Self::MAX as f64 {
            Some(Self(x as i32))
        } else {
            None
        }
    }
    pub fn from_analog_saturating(value: f64) -> Self {
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

pub type Din = Bits<DIN_BITS>;
pub type Dout = Bits<DOUT_BITS>;
