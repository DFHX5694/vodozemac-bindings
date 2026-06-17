# vodozemac-bindings

面向 [vodozemac](https://github.com/matrix-org/vodozemac) 的 C++ 绑定。vodozemac 是 Matrix 端到端加密协议（Olm、Megolm、SAS、ECIES）的 Rust 实现。绑定通过 [cxx](https://github.com/dtolnay/cxx) 生成，以静态库形式供 C++23 项目链接使用。

## 环境要求

| 工具 | 说明 |
|------|------|
| Rust | 稳定版工具链，需包含 `cargo` |
| Meson | ≥ 1.0 |
| Ninja | 推荐作为构建后端 |
| C++ 编译器 | 支持 C++23（`cpp_std=c++23`） |

### Windows 工具链

Meson 根据 **C++ 编译器**（而非仅看操作系统）选择 Rust 目标三元组与静态库路径：

| C++ 工具链 | Rust 目标 | 静态库 |
|------------|-----------|--------|
| MSVC / clang-cl（`get_argument_syntax() == 'msvc'`） | `x86_64-pc-windows-msvc`（默认） | `target/release/vodozemac.lib` |
| MinGW GCC（如 MSYS2 UCRT64） | `x86_64-pc-windows-gnu` | `target/x86_64-pc-windows-gnu/release/libvodozemac.a` |

使用 MinGW 时需额外安装 Rust GNU 目标：

```bash
rustup target add x86_64-pc-windows-gnu
```

在 Windows 上检测到 GCC 时，Meson 会自动为 Cargo 配置 GNU 链接器与 `ar`。

### Linux / macOS

默认 `cargo build --release` 生成 `target/release/libvodozemac.a`。

## 构建

### 快速开始（Windows + MSYS2 UCRT64）

将 `ucrt64/bin` 加入 `PATH` 后执行：

```bash
meson setup build --native-file meson/native/ucrt64.ini
meson compile -C build
```

示例 native 文件 `meson/native/ucrt64.ini` 将编译器指向 MSYS2 UCRT64。若 MSYS2 安装路径不同，请修改其中的路径。

### MSVC

在 **Developer Command Prompt**（或已配置好 `cl` / `clang-cl` 的终端）中：

```bash
meson setup build
meson compile -C build
```

### 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `tests` | `true` | 构建并注册 Catch2 集成测试 |

关闭测试：

```bash
meson setup build -Dtests=false
```

## 运行测试

测试位于 `tests/`，使用 [Catch2](https://github.com/catchorg/Catch2)（系统未安装时通过 Meson wrap 拉取）。

```bash
meson test -C build
```

失败时查看详细日志：

```bash
meson test -C build --print-errorlogs
```

## 在 Meson 项目中使用

将本仓库作为 subproject 或 wrap 引入后，声明依赖：

```meson
vodozemac_dep = dependency('vodozemac-cpp')

my_app = executable(
  'my_app',
  'main.cpp',
  dependencies: [vodozemac_dep],
  cpp_std: 'c++23',
)
```

包含 cxx 生成的头文件：

```cpp
#include "vodozemac/src/lib.rs.h"
```

Meson 会将 `target/cxxbridge` 加入 include 路径，并链接 Cargo 构建的静态库。MinGW 场景下所需的 Windows 系统库（`ws2_32`、`bcrypt` 等）会自动附加到链接参数。

### 单独用 Cargo 构建

```bash
cargo build --release
```

MinGW 需指定 GNU 目标与链接器（Meson 会自动处理）：

```bash
export CARGO_TARGET_DIR="$PWD/target"
export CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER=/path/to/g++
export CARGO_TARGET_X86_64_PC_WINDOWS_GNU_AR=/path/to/ar
cargo build --release --target x86_64-pc-windows-gnu
```

## API 概览

构建完成后，所有类型定义于 `target/cxxbridge/vodozemac/src/lib.rs.h`（相对于项目根目录）。

| 命名空间 | 功能 |
|----------|------|
| `vodozemac` | `base64_encode`、`base64_decode`、`library_version` |
| `vodozemac::types` | Curve25519 / Ed25519 密钥与签名 |
| `vodozemac::olm` | Olm 账户、会话、一次性密钥、设备脱水 |
| `vodozemac::megolm` | 出站 / 入站群组会话、房间加密 |
| `vodozemac::sas` | 短认证字符串（emoji / 数字校验） |
| `vodozemac::ecies` | 二维码登录与集成加密 |

工厂函数返回 `rust::Box<T>`。可能失败的 Rust API 在 bridge 中声明为 `Result<T>`；C++ 侧调用成功时直接得到 `T`，失败时 cxx 会 **`throw rust::Error`**。

### 错误处理

cxx 对 `Result` 的约定（见 [cxx Result 文档](https://cxx.rs/binding/result.html)）：

- Rust 返回 `Err(...)` 时，生成的 C++ 包装代码用 **`throw`** 抛出 **`rust::Error`**（`rust/cxx.h` 中定义，别名 `rust::error`）。
- `rust::Error` **继承自 `std::exception`**，通过 `what()` 取得错误信息；内容来自 Rust 端错误的 `Display` 实现（本项目中多为 `anyhow::Error`）。
- 不是 `std::runtime_error` 等标准库子类，捕获时建议写 `catch (const rust::Error&)` 或 `catch (const std::exception&)`。

```cpp
try {
  auto session = alice->create_outbound_session(config, *bob_identity, *bob_key);
} catch (const rust::Error& e) {
  // e.what() 为 Rust 侧错误描述
}
```

Rust 侧 **panic**（未处理的 `Result` 之外）会记录日志并 `abort`，不会变成 C++ 异常。

### 会话配置

Olm 与 Megolm 协议版本通过配置对象指定：

```cpp
auto olm_cfg = vodozemac::olm::olm_session_config_v1();  // 或 olm_session_config_v2()
auto megolm_cfg = vodozemac::megolm::megolm_session_config_v1();
```

将配置传给 `create_outbound_session`、`create_inbound_session`、`new_group_session` 或 `new_inbound_group_session`。

### 明文：字符串与字节

多数加密 / 签名接口接受 `&str`。二进制安全版本使用 `rust::Slice<const uint8_t>`：

- `sign_bytes`、`encrypt_bytes`、`verify_bytes` 等。

### 使用注意

- **`Session::encrypt`** 返回 `rust::Result`；`Err` 时抛出 `rust::Error`，Rust 侧不再 panic。
- **`generate_fallback_key`** 返回长度为 0 或 1 的 `Vec<Curve25519PublicKey>`（cxx 不支持 `Option`）。
- **`InboundGroupSession::merge`** 返回**新**会话对象，不会就地修改入参。
- **`session_matches`** 仅将预密钥消息与已有会话密钥比对。
- **Pickle 密钥**：vodozemac 原生 pickle 使用固定 32 字节密钥；libolm 兼容 pickle 使用字节切片密钥。

### 未暴露的 API

以下 vodozemac 模块有意未包含在绑定中：

- `pk_encryption`
- `hazmat`（底层原语）

## 项目结构

```
vodozemac-bindings/
├── src/              # Rust 绑定实现（cxx bridge）
├── tests/            # Catch2 C++ 集成测试
├── meson.build       # Cargo custom_target 与依赖导出
├── meson/native/     # 工具链示例配置（如 ucrt64.ini）
├── subprojects/      # Meson wrap（Catch2）
├── build.rs          # cxx bridge 编译规则
└── Cargo.toml        # 静态库 crate "vodozemac"
```

## vodozemac 版本

绑定针对 **vodozemac 0.10.0**，`Cargo.toml` 中启用了 `low-level-api` 与 `experimental-session-config` 特性。

## 许可证

见仓库根目录 [LICENSE](../LICENSE)。
