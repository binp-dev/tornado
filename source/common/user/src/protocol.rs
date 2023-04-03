use crate::{
    config::{ADC_COUNT, MAX_APP_MSG_LEN, MAX_MCU_MSG_LEN},
    values::{AdcPoint, DacPoint, Din, Dout, Value},
};
use core::mem::size_of;
use flatty::{
    flat,
    portable::{le, Bool},
    FlatVec,
};

#[flat(portable = true, sized = false, enum_type = "u8")]
pub enum AppMsg {
    KeepAlive,
    DoutUpdate {
        value: <Dout as Value>::Portable,
    },
    DacState {
        enable: Bool,
    },
    DacData {
        points: FlatVec<<DacPoint as Value>::Portable, le::U16>,
    },
    StatsReset,
}

#[flat(portable = true, sized = false, enum_type = "u8")]
pub enum McuMsg {
    DinUpdate {
        value: <Din as Value>::Portable,
    },
    DacRequest {
        count: le::U32,
    },
    AdcData {
        points: FlatVec<[<AdcPoint as Value>::Portable; ADC_COUNT], le::U16>,
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
    (MAX_APP_MSG_LEN - size_of::<AppMsgTag>() - size_of::<le::U16>())
        / size_of::<<DacPoint as Value>::Portable>();

pub const ADC_MSG_MAX_POINTS: usize =
    (MAX_MCU_MSG_LEN - size_of::<McuMsgTag>() - size_of::<le::U16>())
        / (ADC_COUNT * size_of::<<AdcPoint as Value>::Portable>());
