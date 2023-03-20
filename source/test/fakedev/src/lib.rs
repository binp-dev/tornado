pub mod epics;
pub mod skifio;

pub use epics::Epics;
pub use skifio::SkifioHandle as Skifio;

use mcu::user_main;

pub fn run() -> Skifio {
    let skifio = skifio::bind();

    println!("Starting MCU");
    user_main();
    println!("MCU started");

    skifio
}
