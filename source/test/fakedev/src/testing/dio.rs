use async_std::task::sleep;
use common::values::{Din, Dout};
use epics_ca::ValueChannel as Channel;
use futures::{
    channel::mpsc::{Receiver, Sender},
    SinkExt, StreamExt,
};
use rand::{Rng, SeedableRng};
use rand_xoshiro::Xoshiro128PlusPlus as SomeRng;
use std::{
    io::{stdout, Write},
    time::Duration,
};

pub async fn test_dout(
    mut epics: [Channel<u8>; Dout::SIZE],
    mut device: Receiver<Dout>,
    attempts: usize,
) {
    let mut rng = SomeRng::seed_from_u64(0xdeadbeef);
    let mut value = 0;
    for _ in 0..attempts {
        let i = rng.gen_range(0..Dout::SIZE);
        value ^= 1 << i;
        epics[i].put((value >> i) & 1).unwrap().await.unwrap();
        sleep(Duration::from_millis(40)).await;
        assert_eq!(value, device.next().await.unwrap().into());
        print!(".");
        stdout().flush().unwrap();
    }
}

pub async fn test_din(
    mut epics: [Channel<u8>; Din::SIZE],
    mut device: Sender<Din>,
    attempts: usize,
) {
    let mut rng = SomeRng::seed_from_u64(0xdeadbeef);
    let mut value = 0;
    /*
    for (i, mut chan) in epics.into_iter().enumerate() {
        spawn(async move {
            let mon = chan.subscribe();
            pin_mut!(mon);
            loop {
                println!("di0.B{:X}: {}", i, mon.next().await.unwrap().unwrap());
            }
        });
    }
    */
    let mut monitors = epics
        .iter_mut()
        .map(|chan| Box::pin(chan.subscribe()))
        .collect::<Vec<_>>();
    device.send(Din::default()).await.unwrap();
    sleep(Duration::from_millis(40)).await;
    for mon in monitors.iter_mut() {
        assert_eq!(mon.next().await.unwrap().unwrap(), 0);
    }
    for _ in 0..attempts {
        let i = rng.gen_range(0..Dout::SIZE);
        value ^= 1 << i;
        device.send(Din::try_from(value).unwrap()).await.unwrap();
        sleep(Duration::from_millis(40)).await;
        assert_eq!(monitors[i].next().await.unwrap().unwrap(), (value >> i) & 1);
        print!(":");
        stdout().flush().unwrap();
    }
}
