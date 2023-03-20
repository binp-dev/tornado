use futures::task::{Spawn, SpawnError, SpawnExt};
use std::sync::{
    atomic::{AtomicI32, AtomicU32, Ordering},
    Arc,
};

use crate::{misc::AsyncFlag, variable::WriteVariable};

macro_rules! impl_atomic_variable {
    ($self:ident, $value:ty, $atomic:ty) => {
        pub struct $self {
            event: AsyncFlag,
            value: $atomic,
        }

        impl $self {
            pub fn new(mut variable: WriteVariable<$value>, exec: &impl Spawn) -> Result<Arc<Self>, SpawnError> {
                let self_ = Arc::new(Self {
                    event: AsyncFlag::new(false),
                    value: <$atomic>::new(0),
                });
                let handle = self_.clone();
                exec.spawn(async move {
                    loop {
                        handle.event.take().await;
                        let value = handle.value.load(Ordering::Acquire);
                        variable.write(value).await;
                    }
                })?;
                Ok(self_)
            }

            pub fn write(&self, value: $value) {
                self.value.store(value, Ordering::Release);
                self.event.try_give();
            }
        }
    };
}

impl_atomic_variable!(AtomicVariableU32, u32, AtomicU32);
impl_atomic_variable!(AtomicVariableI32, i32, AtomicI32);
