use base::{entry_point, Context};
use futures::executor::block_on;
use macro_rules_attribute::apply;

pub use base::export;

fn pause() {
    use std::io;
    use std::io::prelude::*;

    let mut stdin = io::stdin();
    let mut stdout = io::stdout();

    // We want the cursor to stay at the end of the line, so we print without a newline and flush manually.
    write!(stdout, "Press `return` to continue...").unwrap();
    stdout.flush().unwrap();

    // Read a single byte and discard
    let _ = stdin.read(&mut [0u8]).unwrap();
}

#[apply(entry_point)]
fn app_main(mut ctx: Context) {
    let mut ai = ctx
        .registry
        .remove("ai")
        .unwrap()
        .downcast_write::<i32>()
        .unwrap();
    let mut ao = ctx
        .registry
        .remove("ao")
        .unwrap()
        .downcast_read::<i32>()
        .unwrap();
    assert!(ctx.registry.is_empty());

    block_on(async {
        loop {
            pause();
            println!("{}", ao.read_current());
            let x = ao.read_next().await;
            println!("x = {}", x);
            ai.write(x).await;
        }
    });
}
