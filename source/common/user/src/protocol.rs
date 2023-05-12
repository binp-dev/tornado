use crate::{
    config::{ADC_COUNT, MAX_APP_MSG_LEN, MAX_MCU_MSG_LEN},
    values::{Din, Dout, PointPortable as Point, UvPortable as Uv},
};
use core::mem::size_of;
use flatty::{
    flat,
    portable::{le, Bool},
    FlatVec,
};

#[flat(portable = true, sized = false, tag_type = "u8")]
pub enum AppMsg {
    KeepAlive,
    DoutUpdate { value: Dout },
    DacState { enable: Bool },
    DacData { points: FlatVec<Point, le::U16> },
    DacAdd { value: Uv },
    StatsReset,
}

#[flat(portable = true, sized = false, tag_type = "u8")]
pub enum McuMsg {
    DinUpdate {
        value: Din,
    },
    DacRequest {
        count: le::U32,
    },
    AdcData {
        points: FlatVec<[Point; ADC_COUNT], le::U16>,
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
    (MAX_APP_MSG_LEN - size_of::<AppMsgTag>() - size_of::<le::U16>()) / size_of::<Point>();

pub const ADC_MSG_MAX_POINTS: usize =
    (MAX_MCU_MSG_LEN - size_of::<McuMsgTag>() - size_of::<le::U16>())
        / (ADC_COUNT * size_of::<Point>());
