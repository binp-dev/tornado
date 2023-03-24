use crate::{
    config::{ADC_COUNT, MAX_APP_MSG_LEN, MAX_MCU_MSG_LEN},
    units::{AdcPoint, DacPoint, Unit},
};
use core::mem::size_of;
use flatty::{flat, portable::le, FlatVec};

#[flat(portable = true, sized = false, enum_type = "u8")]
pub enum AppMsg {
    Connect,
    KeepAlive,
    DoutUpdate {
        value: u8,
    },
    DacState {
        enable: u8,
    },
    DacData {
        points: FlatVec<<DacPoint as Unit>::Portable, le::U16>,
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
        points: FlatVec<[<AdcPoint as Unit>::Portable; ADC_COUNT], le::U16>,
    },
    Error {
        code: u8,
        message: FlatVec<u8, le::U16>,
    },
    Debug {
        message: FlatVec<u8, le::U16>,
    },
}

const fn max(a: usize, b: usize) -> usize {
    if a > b {
        a
    } else {
        b
    }
}

const fn slice_max(slice: &[usize], index: usize) -> usize {
    if index < slice.len() {
        max(slice[index], slice_max(slice, index + 1))
    } else {
        0
    }
}

pub const APP_MSG_MIN_STATIC_SIZE: usize =
    AppMsg::DATA_OFFSET + slice_max(&AppMsg::DATA_MIN_SIZES, 0);

pub const DAC_MSG_MAX_POINTS: usize =
    (MAX_APP_MSG_LEN - size_of::<AppMsgTag>() - size_of::<le::U16>())
        / size_of::<<DacPoint as Unit>::Portable>();
pub const ADC_MSG_MAX_POINTS: usize =
    (MAX_MCU_MSG_LEN - size_of::<McuMsgTag>() - size_of::<le::U16>())
        / (ADC_COUNT * size_of::<<AdcPoint as Unit>::Portable>());
