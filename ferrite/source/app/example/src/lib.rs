use base::{entry_point, Context};
use futures::executor::block_on;
use macro_rules_attribute::apply;

pub use base::export;

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
            let x = ao.read().await;
            println!("x = {}", x);
            ai.write(x).await;
        }
    });
}
