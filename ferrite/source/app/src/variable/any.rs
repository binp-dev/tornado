use super::{
    typing::{Direction, VariableType},
    ReadArrayVariable, ReadVariable, WriteArrayVariable, WriteVariable,
};
use crate::raw;
use std::any::TypeId;

pub struct AnyVariable {
    raw: raw::Variable,
    type_: VariableType,
    dir: Direction,
}

impl AnyVariable {
    pub(crate) fn new(raw: raw::Variable) -> Self {
        let raw_type = raw.lock().data_type();
        Self {
            raw,
            type_: VariableType::from_raw(raw_type),
            dir: Direction::from_raw(raw_type.dir),
        }
    }

    pub fn name(&self) -> String {
        self.raw.lock().name().to_str().unwrap().to_owned()
    }

    pub fn direction(&self) -> Direction {
        self.dir
    }
    pub fn data_type(&self) -> VariableType {
        self.type_
    }

    pub fn downcast_read<T: Copy + 'static + 'static>(self) -> Option<ReadVariable<T>> {
        match self.dir {
            Direction::Read => match self.type_ {
                VariableType::Scalar { scal_type } => {
                    if scal_type.type_id() == Some(TypeId::of::<T>()) {
                        Some(ReadVariable::from_raw(self.raw))
                    } else {
                        None
                    }
                }
                _ => None,
            },
            Direction::Write => None,
        }
    }
    pub fn downcast_write<T: Copy + 'static + 'static>(self) -> Option<WriteVariable<T>> {
        match self.dir {
            Direction::Read => None,
            Direction::Write => match self.type_ {
                VariableType::Scalar { scal_type } => {
                    if scal_type.type_id() == Some(TypeId::of::<T>()) {
                        Some(WriteVariable::from_raw(self.raw))
                    } else {
                        None
                    }
                }
                _ => None,
            },
        }
    }
    pub fn downcast_read_array<T: Copy + 'static + 'static>(self) -> Option<ReadArrayVariable<T>> {
        match self.dir {
            Direction::Read => match self.type_ {
                VariableType::Array { scal_type, max_len } => {
                    if scal_type.type_id() == Some(TypeId::of::<T>()) {
                        Some(ReadArrayVariable::from_raw(self.raw, max_len))
                    } else {
                        None
                    }
                }
                _ => None,
            },
            Direction::Write => None,
        }
    }
    pub fn downcast_write_array<T: Copy + 'static + 'static>(self) -> Option<WriteArrayVariable<T>> {
        match self.dir {
            Direction::Read => None,
            Direction::Write => match self.type_ {
                VariableType::Array { scal_type, max_len } => {
                    if scal_type.type_id() == Some(TypeId::of::<T>()) {
                        Some(WriteArrayVariable::from_raw(self.raw, max_len))
                    } else {
                        None
                    }
                }
                _ => None,
            },
        }
    }
}

pub trait Downcast<V> {
    fn downcast(self) -> Option<V>;
}

impl<T: Copy + 'static> Downcast<ReadVariable<T>> for AnyVariable {
    fn downcast(self) -> Option<ReadVariable<T>> {
        self.downcast_read::<T>()
    }
}

impl<T: Copy + 'static> Downcast<WriteVariable<T>> for AnyVariable {
    fn downcast(self) -> Option<WriteVariable<T>> {
        self.downcast_write::<T>()
    }
}

impl<T: Copy + 'static> Downcast<ReadArrayVariable<T>> for AnyVariable {
    fn downcast(self) -> Option<ReadArrayVariable<T>> {
        self.downcast_read_array::<T>()
    }
}

impl<T: Copy + 'static> Downcast<WriteArrayVariable<T>> for AnyVariable {
    fn downcast(self) -> Option<WriteArrayVariable<T>> {
        self.downcast_write_array::<T>()
    }
}
