use crate::{
    config::{AI_COUNT, MAX_APP_MSG_LEN, MAX_MCU_MSG_LEN},
    values::{Di, Do, Point, Uv},
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
    DoUpdate { value: Do },
    AoState { enable: Bool },
    AoData { points: FlatVec<Point, u16> },
    AoAdd { value: Uv },
    StatsReset,
}

#[flat(sized = false, tag_type = "u8")]
pub enum McuMsg {
    DiUpdate {
        value: Di,
    },
    AoRequest {
        count: u32,
    },
    AiData {
        points: FlatVec<[Point; AI_COUNT], u16>,
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
pub const AO_MSG_MAX_POINTS: usize = (floor_mul(MAX_APP_MSG_LEN, AppMsg::ALIGN)
    - ceil_mul(size_of::<AppMsgTag>(), AppMsg::ALIGN)
    - ceil_mul(size_of::<u16>(), Point::ALIGN))
    / size_of::<Point>();

/// Calculate `McuMsg::AdcData::points` capacity based on its layout.
pub const AI_MSG_MAX_POINTS: usize = (floor_mul(MAX_MCU_MSG_LEN, McuMsg::ALIGN)
    - ceil_mul(size_of::<McuMsgTag>(), McuMsg::ALIGN)
    - ceil_mul(size_of::<u16>(), Point::ALIGN))
    / (AI_COUNT * size_of::<Point>());
