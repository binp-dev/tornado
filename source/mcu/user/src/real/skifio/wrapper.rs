use super::raw;
use crate::{hal::RetCode, println, Error};
use alloc::boxed::Box;
use core::{
    cell::UnsafeCell,
    ffi::c_void,
    ptr::{self, NonNull},
    sync::atomic::{AtomicBool, AtomicU8, Ordering},
    time::Duration,
};
use freertos::InterruptContext;
use lazy_static::lazy_static;

pub use raw::{Ain, Aout, Din, Dout, XferIn, XferOut, DIN_SIZE, DOUT_SIZE};

pub type AtomicDin = AtomicU8;
pub type AtomicDout = AtomicU8;

lazy_static! {
    static ref SKIFIO: GlobalSkifio = GlobalSkifio::new();
}

struct GlobalSkifio {
    acquired: AtomicBool,
    state: UnsafeCell<SkifioState>,
}
unsafe impl Sync for GlobalSkifio {}

pub trait DinHandler: FnMut(&mut InterruptContext, Din) + Send + 'static {}
impl<T: FnMut(&mut InterruptContext, Din) + Send + 'static> DinHandler for T {}

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
        if self.acquired.fetch_and(false, Ordering::SeqCst) {
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
    fn set_din_handler(&mut self, handler_opt: Option<Box<dyn DinHandler + Send>>) -> *mut c_void {
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

    pub fn set_dac_state(&mut self, enabled: bool) -> Result<(), Error> {
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
    pub fn dac_state(&self) -> bool {
        self.state().dac_state
    }

    pub fn wait_ready(&mut self, timeout: Option<Duration>) -> Result<(), Error> {
        unsafe { raw::skifio_wait_ready(timeout.into()) }.into()
    }
    pub fn transfer(&mut self, out: XferOut) -> Result<XferIn, Error> {
        let mut in_ = XferIn::default();
        match unsafe { raw::skifio_transfer(&out as *const _, &mut in_ as *mut _) } {
            RetCode::Success => Ok(in_),
            r => Err(Error::Hal(r)),
        }
    }

    pub fn write_dout(&mut self, dout: Dout) -> Result<(), Error> {
        unsafe { raw::skifio_dout_write(dout) }.into()
    }

    pub fn read_din(&mut self) -> Din {
        unsafe { raw::skifio_din_read() }
    }
    pub fn subscribe_din<F: DinHandler + Send + 'static>(
        &mut self,
        callback: Option<F>,
    ) -> Result<(), Error> {
        Into::<Result<(), Error>>::into(unsafe { raw::skifio_din_unsubscribe() })?;
        self.state_mut().set_din_handler(None);

        if let Some(cb) = callback {
            let cb_ptr = self.state_mut().set_din_handler(Some(Box::new(cb)));
            unsafe { raw::skifio_din_subscribe(Self::din_callback as *mut _, cb_ptr) }.into()
        } else {
            Ok(())
        }
    }
    extern "C" fn din_callback(data: *mut c_void, value: Din) {
        let mut context = InterruptContext::new();
        unsafe { (**(data as *const *mut dyn DinHandler))(&mut context, value) };
    }
}
