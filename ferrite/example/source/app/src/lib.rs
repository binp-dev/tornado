use base::{entry_point, Context};
use futures::{executor::block_on, join};
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
    let mut aai = ctx
        .registry
        .remove("aai")
        .unwrap()
        .downcast_write_array::<i32>()
        .unwrap();
    let mut aao = ctx
        .registry
        .remove("aao")
        .unwrap()
        .downcast_read_array::<i32>()
        .unwrap();
    assert!(ctx.registry.is_empty());

    block_on(async move {
        join!(
            async move {
                loop {
                    let x = ao.read().await;
                    println!("[ao -> ai]: {}", x);
                    ai.write(x).await;
                }
            },
            async move {
                loop {
                    let mut buf = vec![0; usize::max(aai.max_len(), aao.max_len())];
                    let len = aao.read_to_slice(&mut buf).await.unwrap();
                    println!("[aao -> aai]: (len={}){:?}", len, &buf[..len]);
                    aai.write_from_slice(&buf[..len]).await;
                }
            }
        )
    });
}
