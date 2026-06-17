# vodozemac-bindings

C++ bindings for [vodozemac](https://github.com/matrix-org/vodozemac) — the Rust implementation of the Matrix end-to-end encryption protocol (Olm, Megolm, SAS, ECIES). The bindings are generated with [cxx](https://github.com/dtolnay/cxx) and built as a static library consumed from C++23 code.

## Requirements

| Tool | Version / notes |
|------|-----------------|
| Rust | Stable toolchain with `cargo` |
| Meson | ≥ 1.0 |
| Ninja | Recommended build backend |
| C++ compiler | C++23 (`cpp_std=c++23`) |

### Windows toolchains

Meson selects the Rust target and static library name from the **C++ compiler**, not from the host OS alone:

| C++ toolchain | Rust target | Static library |
|---------------|-------------|----------------|
| MSVC / clang-cl (`get_argument_syntax() == 'msvc'`) | `x86_64-pc-windows-msvc` (default) | `target/release/vodozemac.lib` |
| MinGW GCC (e.g. MSYS2 UCRT64) | `x86_64-pc-windows-gnu` | `target/x86_64-pc-windows-gnu/release/libvodozemac.a` |

For MinGW builds you also need the Rust GNU target:

```bash
rustup target add x86_64-pc-windows-gnu
```

Meson configures the GNU linker automatically when it detects GCC on Windows.

### Linux / macOS

Default `cargo build --release` produces `target/release/libvodozemac.a`.

## Building

### Quick start (MSYS2 UCRT64 on Windows)

Ensure `ucrt64/bin` is on `PATH`, then:

```bash
meson setup build --native-file meson/native/ucrt64.ini
meson compile -C build
```

The example native file points compilers to MSYS2 UCRT64. Adjust paths in `meson/native/ucrt64.ini` if your MSYS2 install lives elsewhere.

### MSVC

Open a **Developer Command Prompt** (or any shell where `cl` / `clang-cl` is available) and run:

```bash
meson setup build
meson compile -C build
```

### Build options

| Option | Default | Description |
|--------|---------|-------------|
| `tests` | `true` | Build and register Catch2 integration tests |

Disable tests:

```bash
meson setup build -Dtests=false
```

## Running tests

Tests live under `tests/` and use [Catch2](https://github.com/catchorg/Catch2) (fetched via Meson wrap when not installed system-wide).

```bash
meson test -C build
```

Verbose output on failure:

```bash
meson test -C build --print-errorlogs
```

## Using in your Meson project

Add this repository as a subproject or wrap, then depend on the overridden package name:

```meson
vodozemac_dep = dependency('vodozemac-cpp')

my_app = executable(
  'my_app',
  'main.cpp',
  dependencies: [vodozemac_dep],
  cpp_std: 'c++23',
)
```

Include the generated cxx bridge header:

```cpp
#include "vodozemac/src/lib.rs.h"
```

Meson exposes `target/cxxbridge` as an include directory and links the static library built by Cargo. On MinGW, required Windows system libraries (`ws2_32`, `bcrypt`, etc.) are added automatically.

### Manual Cargo build

You can build the Rust side alone:

```bash
cargo build --release
```

For MinGW, pass the GNU target and linker (Meson does this for you):

```bash
export CARGO_TARGET_DIR="$PWD/target"
export CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER=/path/to/g++
export CARGO_TARGET_X86_64_PC_WINDOWS_GNU_AR=/path/to/ar
cargo build --release --target x86_64-pc-windows-gnu
```

## API overview

All types live in generated header `target/cxxbridge/vodozemac/src/lib.rs.h` (path relative to project root after build).

| Namespace | Purpose |
|-----------|---------|
| `vodozemac` | `base64_encode`, `base64_decode`, `library_version` |
| `vodozemac::types` | Curve25519 / Ed25519 keys and signatures |
| `vodozemac::olm` | Olm account, session, one-time keys, dehydration |
| `vodozemac::megolm` | Outbound / inbound group sessions, room encryption |
| `vodozemac::sas` | Short Authentication String (emoji / decimal verification) |
| `vodozemac::ecies` | QR login and integrated encryption |

Factory functions return `rust::Box<T>`. Fallible Rust APIs are declared as `Result<T>` in the bridge; on success C++ receives `T` directly, and on failure cxx **`throw`s `rust::Error`**.

### Error handling

cxx’s `Result` mapping (see [cxx Result docs](https://cxx.rs/binding/result.html)):

- When Rust returns `Err(...)`, the generated C++ shim **`throw`s `rust::Error`** (defined in `rust/cxx.h`, alias `rust::error`).
- `rust::Error` **inherits from `std::exception`**; `what()` returns the message from the Rust error’s `Display` impl (mostly `anyhow::Error` in this project).
- It is not `std::runtime_error` or another standard subclass — prefer `catch (const rust::Error&)` or `catch (const std::exception&)`.

```cpp
try {
  auto session = alice->create_outbound_session(config, *bob_identity, *bob_key);
} catch (const rust::Error& e) {
  // e.what() holds the Rust error description
}
```

A Rust **panic** (outside unhandled `Result`) is logged and the process **aborts**; it does not become a C++ exception.

### Session configuration

Olm and Megolm protocol versions are set via config objects:

```cpp
auto olm_cfg = vodozemac::olm::olm_session_config_v1();  // or olm_session_config_v2()
auto megolm_cfg = vodozemac::megolm::megolm_session_config_v1();
```

Pass the config to `create_outbound_session`, `create_inbound_session`, `new_group_session`, or `new_inbound_group_session`.

### Plaintext: string vs bytes

Many encrypt/sign APIs accept `&str`. Binary-safe variants use `rust::Slice<const uint8_t>`:

- `sign_bytes`, `encrypt_bytes`, `verify_bytes`, etc.

### Behaviour notes

- **`Session::encrypt`** returns `rust::Result`; on `Err` it throws `rust::Error` instead of panicking in Rust.
- **`generate_fallback_key`** returns `Vec<Curve25519PublicKey>` with length 0 or 1 (cxx has no `Option`).
- **`InboundGroupSession::merge`** returns a **new** session; inputs are not updated in place.
- **`session_matches`** only compares pre-key messages against existing session keys.
- **Pickle keys**: vodozemac-native pickles use a fixed 32-byte key; libolm compatibility pickles use a byte slice key.

### Not exposed

The following vodozemac areas are intentionally omitted from these bindings:

- `pk_encryption`
- `hazmat` (low-level primitives)

## Project layout

```
vodozemac-bindings/
├── src/              # Rust binding implementation (cxx bridge)
├── tests/            # Catch2 C++ integration tests
├── meson.build       # Cargo custom_target + dependency export
├── meson/native/     # Example toolchain files (e.g. ucrt64.ini)
├── subprojects/      # Meson wraps (Catch2)
├── build.rs          # cxx bridge compile rules
└── Cargo.toml        # staticlib crate "vodozemac"
```

## vodozemac version

Bindings target **vodozemac 0.10.0** with features `low-level-api` and `experimental-session-config` enabled in `Cargo.toml`.

## License

See [LICENSE](../LICENSE) in the repository root.
