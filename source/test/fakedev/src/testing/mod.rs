pub mod adc;
pub mod dac;
pub mod dio;

use common::values::{Point, Value};

extern "C" {
    fn user_sample_intr();
}

fn scale(x: f64) -> f64 {
    let (min, max) = (
        Point::try_from_base(Point::MIN).unwrap().into_analog(),
        Point::try_from_base(Point::MAX).unwrap().into_analog(),
    );
    (x + 1.0) / 2.0 * (max - min) + min
}
