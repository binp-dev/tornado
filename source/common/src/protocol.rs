use crate::config::{PointPortable, ADC_COUNT, DAC_COUNT, MAX_APP_MSG_LEN, MAX_MCU_MSG_LEN};
use core::mem::size_of;
use flatty::{flat, portable::le, FlatVec};

#[flat(portable = true, sized = false, enum_type = "u8")]
pub enum AppMsg {
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

pub const DAC_MSG_MAX_POINTS: usize =
    (MAX_APP_MSG_LEN - size_of::<AppMsgTag>()) / (DAC_COUNT * size_of::<PointPortable>());
pub const ADC_MSG_MAX_POINTS: usize =
    (MAX_MCU_MSG_LEN - size_of::<McuMsgTag>()) / (ADC_COUNT * size_of::<PointPortable>());
