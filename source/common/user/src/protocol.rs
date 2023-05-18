use crate::{
    config::{ADC_COUNT, MAX_APP_MSG_LEN, MAX_MCU_MSG_LEN},
    values::{Din, Dout, Point, Uv},
};
use core::mem::size_of;
use flatty::{
    flat,
    portable::Bool,
    traits::FlatBase,
    utils::{ceil_mul, floor_mul},
    FlatVec,
};

#[flat(sized = false, tag_type = "u8")]
pub enum AppMsg {
    KeepAlive,
    DoutUpdate { value: Dout },
    DacState { enable: Bool },
    DacData { points: FlatVec<Point, u16> },
    DacAdd { value: Uv },
    StatsReset,
}

#[flat(sized = false, tag_type = "u8")]
pub enum McuMsg {
    DinUpdate {
        value: Din,
    },
    DacRequest {
        count: u32,
    },
    AdcData {
        points: FlatVec<[Point; ADC_COUNT], u16>,
    },
    Error {
        code: u8,
        message: FlatVec<u8, u16>,
    },
    Debug {
        message: FlatVec<u8, u16>,
    },
}

/// Calculate `AppMsg::DacData::points` capacity based on its layout.
pub const DAC_MSG_MAX_POINTS: usize = (floor_mul(MAX_APP_MSG_LEN, AppMsg::ALIGN)
    - ceil_mul(size_of::<AppMsgTag>(), AppMsg::ALIGN)
    - ceil_mul(size_of::<u16>(), Point::ALIGN))
    / size_of::<Point>();

/// Calculate `McuMsg::AdcData::points` capacity based on its layout.
pub const ADC_MSG_MAX_POINTS: usize = (floor_mul(MAX_MCU_MSG_LEN, McuMsg::ALIGN)
    - ceil_mul(size_of::<McuMsgTag>(), McuMsg::ALIGN)
    - ceil_mul(size_of::<u16>(), Point::ALIGN))
    / (ADC_COUNT * size_of::<Point>());
