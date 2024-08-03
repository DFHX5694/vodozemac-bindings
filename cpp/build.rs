use cxx_build::CFG;

fn main() {
    CFG.include_prefix = "vodozemac";

    let sources = [
        "build.rs",
        "src/lib.rs",
        "src/account.rs",
        "src/group_sessions.rs",
        "src/sas.rs",
        "src/session.rs",
        "src/types.rs",
    ];

    for source in sources {
        println!("cargo::rerun-if-changed={}", source);
    }

    cxx_build::bridge("src/lib.rs").compile("vodozemac");
}
