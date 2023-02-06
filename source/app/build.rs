use std::env;
use std::fs;
use std::path::Path;

fn copy(src: &Path, dst: &Path) {
    let abs_src = Path::new(&env::var_os("TARGET_DIR").unwrap()).join(src);
    let abs_dst = Path::new(&env::var_os("OUT_DIR").unwrap()).join(dst);
    fs::copy(&abs_src, abs_dst).unwrap();
    println!("cargo:rerun-if-changed={}", abs_src.display());
}

fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    copy(Path::new("tornado/ipp/rust/src/proto.rs"), Path::new("proto.rs"));
    copy(Path::new("tornado/config/rust/config.rs"), Path::new("config.rs"));
}
