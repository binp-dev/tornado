fn main() {
    println!("cargo:rustc-link-lib=static={{codegen}}_test");
    println!("cargo:rustc-link-search=native=../c/build/");
    println!("cargo:rerun-if-changed=../c/");
}
