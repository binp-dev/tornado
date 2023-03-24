pub mod adc;
pub mod dac;

use common::units::Voltage;

extern "C" {
    fn user_sample_intr();
}

fn scale<T: Voltage>(x: f64) -> f64 {
    let (min, max) = (T::MIN.to_voltage(), T::MAX.to_voltage());
    (x + 1.0) / 2.0 * (max - min) + min
}
