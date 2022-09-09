use async_ringbuf::AsyncHeapRb;
use flatty::{
    make_flat,
    portable::{le, NativeCast},
    FlatVec,
};
use futures::join;

use super::{read::MsgReadError, MsgReader, MsgWriter};

#[make_flat(sized = false, portable = true)]
enum TestMsg {
    #[default]
    A,
    B(le::I32),
    C(FlatVec<le::I32, le::U16>),
}

#[async_std::test]
async fn test() {
    const MAX_SIZE: usize = 32;
    let (prod, cons) = AsyncHeapRb::<u8>::new(17).split();
    join!(
        async move {
            let mut writer = MsgWriter::<TestMsg, _>::new(prod, MAX_SIZE);

            writer.init_default_msg().unwrap().write().await.unwrap();

            {
                let mut guard = writer.init_default_msg().unwrap();
                guard.reset_tag(TestMsgTag::B).unwrap();
                if let TestMsgMut::B(x) = guard.as_mut() {
                    *x = 123456.into();
                } else {
                    unreachable!();
                }
                guard.write().await.unwrap();
            }

            {
                let mut guard = writer.new_uninit_msg();
                TestMsg::set_tag(&mut guard, TestMsgTag::C).unwrap();
                let mut guard = guard.validate().unwrap();
                if let TestMsgMut::C(vec) = guard.as_mut() {
                    assert_eq!(vec.extend_from_iter((0..7).into_iter().map(|x| x.into())), 7);
                } else {
                    unreachable!();
                }
                guard.write().await.unwrap();
            }
        },
        async move {
            let mut reader = MsgReader::<TestMsg, _>::new(cons, MAX_SIZE);

            {
                let guard = reader.read_msg().await.unwrap();
                match guard.as_ref() {
                    TestMsgRef::A => (),
                    _ => panic!(),
                }
            }

            {
                let guard = reader.read_msg().await.unwrap();
                match guard.as_ref() {
                    TestMsgRef::B(x) => assert_eq!(x.to_native(), 123456),
                    _ => panic!(),
                }
            }

            {
                let guard = reader.read_msg().await.unwrap();
                match guard.as_ref() {
                    TestMsgRef::C(v) => {
                        let vn = v.iter().map(|x| x.to_native()).collect::<Vec<_>>();
                        assert_eq!(&vn, &[0, 1, 2, 3, 4, 5, 6]);
                    }
                    _ => panic!(),
                }
            }

            match reader.read_msg().await.err().unwrap() {
                MsgReadError::Eof => (),
                _ => panic!(),
            }
        },
    );
}
