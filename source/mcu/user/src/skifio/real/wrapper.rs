use super::raw;
use crate::{hal::RetCode, println, Error};
use alloc::boxed::Box;
use core::{
    cell::UnsafeCell,
    ffi::c_void,
    ptr::{self, NonNull},
    sync::atomic::{AtomicBool, Ordering},
    time::Duration,
};
use freertos::InterruptContext;
use lazy_static::lazy_static;

use crate::skifio::{Din, DinHandler, Dout, SkifioIface, XferIn, XferOut};

lazy_static! {
    static ref SKIFIO: GlobalSkifio = GlobalSkifio::new();
}

struct GlobalSkifio {
    acquired: AtomicBool,
    state: UnsafeCell<SkifioState>,
}
unsafe impl Sync for GlobalSkifio {}

struct SkifioState {
    dac_state: bool,
    din_handler: Option<NonNull<dyn DinHandler>>,
}
unsafe impl Send for SkifioState {}

impl GlobalSkifio {
    fn new() -> Self {
        println!("SkifIO driver init");
        assert_eq!(unsafe { raw::skifio_init() }, RetCode::Success);
        Self {
            acquired: AtomicBool::new(false),
            state: UnsafeCell::new(SkifioState {
                dac_state: false,
                din_handler: None,
            }),
        }
    }

    fn try_acquire(&self) -> Option<Skifio> {
        if !self.acquired.fetch_or(true, Ordering::AcqRel) {
            Some(Skifio::new())
        } else {
            None
        }
    }

    #[allow(clippy::mut_from_ref)]
    unsafe fn state(&self) -> &mut SkifioState {
        unsafe { &mut *self.state.get() }
    }
}

impl SkifioState {
    fn set_din_handler(&mut self, handler_opt: Option<Box<dyn DinHandler>>) -> *mut c_void {
        if let Some(ptr) = self.din_handler.take() {
            let _ = unsafe { Box::from_raw(ptr.as_ptr()) };
        }
        if let Some(handler) = handler_opt {
            self.din_handler = NonNull::new(Box::into_raw(handler));
            self.din_handler.as_mut().unwrap() as *mut _ as *mut c_void
        } else {
            ptr::null_mut()
        }
    }
}

pub fn handle() -> Option<Skifio> {
    SKIFIO.try_acquire()
}

impl Drop for GlobalSkifio {
    fn drop(&mut self) {
        assert_eq!(unsafe { raw::skifio_deinit() }, RetCode::Success);
        unsafe { self.state().set_din_handler(None) };
    }
}

pub struct Skifio {
    _unused: [u8; 0],
}

impl Skifio {
    fn new() -> Self {
        Self { _unused: [] }
    }
    fn state(&self) -> &SkifioState {
        unsafe { SKIFIO.state() }
    }
    fn state_mut(&mut self) -> &mut SkifioState {
        unsafe { SKIFIO.state() }
    }

    extern "C" fn din_callback(data: *mut c_void, value: Din) {
        let mut context = InterruptContext::new();
        unsafe { (**(data as *const *mut dyn DinHandler))(&mut context, value) };
    }
}

impl SkifioIface for Skifio {
    fn set_dac_state(&mut self, enabled: bool) -> Result<(), Error> {
        if self.state().dac_state != enabled {
            let r = if enabled {
                unsafe { raw::skifio_dac_enable() }
            } else {
                unsafe { raw::skifio_dac_disable() }
            };
            if r == RetCode::Success {
                self.state_mut().dac_state = enabled;
            }
            r.into()
        } else {
            Ok(())
        }
    }
    fn dac_state(&self) -> bool {
        self.state().dac_state
    }

    fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error> {
        unsafe { raw::skifio_wait_ready(timeout.into()) }.into()
    }
    fn transfer(&mut self, out: XferOut) -> Result<XferIn, Error> {
        let mut in_ = XferIn::default();
        let r = unsafe { raw::skifio_transfer(&out as *const _, &mut in_ as *mut _) };
        Result::<(), Error>::from(r).map(|()| in_)
    }

    fn write_dout(&mut self, dout: Dout) -> Result<(), Error> {
        unsafe { raw::skifio_dout_write(dout) }.into()
    }

    fn read_din(&mut self) -> Din {
        unsafe { raw::skifio_din_read() }
    }
    fn subscribe_din(&mut self, callback: Option<Box<dyn DinHandler>>) -> Result<(), Error> {
        Into::<Result<(), Error>>::into(unsafe { raw::skifio_din_unsubscribe() })?;
        self.state_mut().set_din_handler(None);

        if let Some(cb) = callback {
            let cb_ptr = self.state_mut().set_din_handler(Some(cb));
            unsafe { raw::skifio_din_subscribe(Self::din_callback as *mut _, cb_ptr) }.into()
        } else {
            Ok(())
        }
    }
}
