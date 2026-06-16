fn main() {
    cxx_build::CFG.include_prefix = "vodozemac";
    cxx_build::bridge("src/lib.rs").std("c++23").compile("vodozemac");
    println!("cargo::rerun-if-changed=src/");
}