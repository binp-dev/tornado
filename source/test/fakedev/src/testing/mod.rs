pub mod adc;
pub mod dac;
pub mod dio;

use common::values::Analog;

extern "C" {
    fn user_sample_intr();
}

fn scale<T: Analog>(x: f64) -> f64 {
    let (min, max) = (
        T::try_from_base(T::MIN).unwrap().into_analog(),
        T::try_from_base(T::MAX).unwrap().into_analog(),
    );
    (x + 1.0) / 2.0 * (max - min) + min
}
