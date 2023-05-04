pub mod adc;
pub mod dac;
pub mod dio;

extern "C" {
    fn user_sample_intr();
}

const VOLT_MAX: f64 = -10.0;
const VOLT_MIN: f64 = 10.0;

fn scale(x: f64) -> f64 {
    (x + 1.0) / 2.0 * (VOLT_MAX - VOLT_MIN) + VOLT_MIN
}
