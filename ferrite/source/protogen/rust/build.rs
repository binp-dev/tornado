fn main() {
    println!("cargo:rustc-link-lib=static={{protogen}}_test");
    println!("cargo:rustc-link-search=native=../c/build/");
    println!("cargo:rerun-if-changed=../c/");
}
