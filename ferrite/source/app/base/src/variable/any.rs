use super::{
    type_::{Direction, VariableType},
    ReadArrayVariable, ReadVariable, WriteArrayVariable, WriteVariable,
};
use crate::sys;
use std::any::TypeId;

pub struct AnyVariable {
    raw: sys::VarLock,
    type_: VariableType,
    dir: Direction,
}

impl AnyVariable {
    pub unsafe fn new(raw: sys::Var) -> Self {
        let raw_lock = sys::VarLock::new(raw);
        let raw_type = raw_lock.lock().type_();
        Self {
            raw: raw_lock,
            type_: VariableType::from_raw(raw_type),
            dir: Direction::from_raw(raw_type.dir),
        }
    }
    pub fn downcast_read<T: Copy + 'static>(self) -> Option<ReadVariable<T>> {
        match self.dir {
            Direction::Read => match self.type_ {
                VariableType::Scalar { scal_type } => {
                    if scal_type.type_id() == Some(TypeId::of::<T>()) {
                        unsafe { Some(ReadVariable::from_raw(self.raw.into_inner())) }
                    } else {
                        None
                    }
                }
                _ => None,
            },
            Direction::Write => None,
        }
    }
    pub fn downcast_write<T: Copy + 'static>(self) -> Option<WriteVariable<T>> {
        match self.dir {
            Direction::Read => None,
            Direction::Write => match self.type_ {
                VariableType::Scalar { scal_type } => {
                    if scal_type.type_id() == Some(TypeId::of::<T>()) {
                        unsafe { Some(WriteVariable::from_raw(self.raw.into_inner())) }
                    } else {
                        None
                    }
                }
                _ => None,
            },
        }
    }
    pub fn downcast_read_array<T: Copy + 'static>(self) -> Option<ReadArrayVariable<T>> {
        match self.dir {
            Direction::Read => match self.type_ {
                VariableType::Array { scal_type, max_len } => {
                    if scal_type.type_id() == Some(TypeId::of::<T>()) {
                        unsafe { Some(ReadArrayVariable::from_raw(self.raw.into_inner(), max_len)) }
                    } else {
                        None
                    }
                }
                _ => None,
            },
            Direction::Write => None,
        }
    }
    pub fn downcast_write_array<T: Copy + 'static>(self) -> Option<WriteArrayVariable<T>> {
        match self.dir {
            Direction::Read => None,
            Direction::Write => match self.type_ {
                VariableType::Array { scal_type, max_len } => {
                    if scal_type.type_id() == Some(TypeId::of::<T>()) {
                        unsafe {
                            Some(WriteArrayVariable::from_raw(self.raw.into_inner(), max_len))
                        }
                    } else {
                        None
                    }
                }
                _ => None,
            },
        }
    }
}
