use std::alloc::{alloc, Layout};

pub fn vec_aligned(src: &[u8], align: usize) -> Vec<u8> {
    unsafe {
        let ptr = alloc(Layout::from_size_align(src.len(), align).unwrap());
        let mut vec = Vec::<u8>::from_raw_parts(ptr, src.len(), src.len());
        vec.copy_from_slice(src);
        vec
    }
}
