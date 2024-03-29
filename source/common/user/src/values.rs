use crate::config::{DI_BITS, DO_BITS};
use core::{
    fmt::Debug,
    sync::atomic::{AtomicI32, AtomicU32, AtomicU8, Ordering},
};
use flatty::{error::ErrorKind, flat, traits::FlatValidate, Flat, Portable};

pub type Uv = i32;
pub type AtomicUv = AtomicI32;

#[derive(Default)]
pub struct AtomicF32(AtomicU32);

impl AtomicF32 {
    pub fn load(&self, ord: Ordering) -> f32 {
        f32::from_le_bytes(self.0.load(ord).to_le_bytes())
    }
    pub fn store(&self, val: f32, ord: Ordering) {
        self.0.store(u32::from_le_bytes(val.to_le_bytes()), ord)
    }
}

#[flat]
#[derive(Clone, Copy, Default, Debug, Eq, PartialEq, Ord, PartialOrd)]
pub struct Point(i32);
pub enum PointOpt {
    Uv(Uv),
    Sep,
}
impl Point {
    const NICHE: i32 = 1;

    pub const MAX_UV: Uv = Uv::MAX;
    pub const MIN_UV: Uv = Uv::MIN + Self::NICHE;

    pub const SEP: Self = Self(i32::MIN);

    pub fn from_uv(uv: Uv) -> Self {
        Point(uv.max(Self::MIN_UV))
    }

    pub fn into_opt(self) -> PointOpt {
        if self.0 >= Self::MIN_UV {
            PointOpt::Uv(self.0)
        } else {
            PointOpt::Sep
        }
    }
}

pub const VOLT_EPS: f64 = 1e-6;
pub fn uv_to_volt(uv: Uv) -> f64 {
    uv as f64 * 1e-6
}
pub fn try_volt_to_uv(value: f64) -> Option<Uv> {
    let x = value * 1e6;
    if x >= Uv::MIN as f64 && x <= Uv::MAX as f64 {
        Some(x as i32)
    } else {
        None
    }
}
pub fn volt_to_uv_saturating(value: f64) -> Uv {
    (value * 1e6).clamp(Uv::MIN as f64, Uv::MAX as f64) as i32
}

#[repr(transparent)]
#[derive(Clone, Copy, Default, Debug, Eq, PartialEq)]
pub struct Bits<const N: usize>(u8);
impl<const N: usize> Bits<N> {
    pub const SIZE: usize = N;
    const MAX: u8 = u8::MAX >> (8 - N);
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
unsafe impl<const N: usize> FlatValidate for Bits<N> {
    unsafe fn validate_unchecked(bytes: &[u8]) -> Result<(), flatty::Error> {
        if *unsafe { bytes.get_unchecked(0) } <= Self::MAX {
            Ok(())
        } else {
            Err(flatty::Error {
                kind: ErrorKind::InvalidData,
                pos: 0,
            })
        }
    }
}
unsafe impl<const N: usize> Flat for Bits<N> {}
unsafe impl<const N: usize> Portable for Bits<N> {}
pub type AtomicBits = AtomicU8;

pub type Di = Bits<DI_BITS>;
pub type Do = Bits<DO_BITS>;
