use crate::config::{PointPortable, ADC_COUNT, DAC_COUNT};
use flatty::{flat, portable::le, FlatVec};

#[flat(portable = true, sized = false, enum_type = "u8")]
pub enum AppMsg {
    Empty,
    Connect,
    KeepAlive,
    DoutUpdate {
        value: u8,
    },
    DacMode {
        enable: u8,
    },
    DacData {
        points: FlatVec<[PointPortable; DAC_COUNT], le::U16>,
    },
    StatsReset,
}

#[flat(portable = true, sized = false, enum_type = "u8")]
pub enum McuMsg {
    Empty,
    DinUpdate {
        value: u8,
    },
    DacRequest {
        count: le::U32,
    },
    AdcData {
        points: FlatVec<[PointPortable; ADC_COUNT], le::U16>,
    },
    Error {
        code: u8,
        message: FlatVec<u8, le::U16>,
    },
    Debug {
        message: FlatVec<u8, le::U16>,
    },
}
