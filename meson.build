project('vodozemac-cpp', 'cpp', version: '0.1.0',
  default_options: [
    'cpp_std=c++23',
    'warning_level=3',
  ],
)

cpp = meson.get_compiler('cpp')
cargo = find_program('cargo', required: true)

project_source = meson.current_source_dir()
cargo_target_dir = meson.current_build_dir() / 'cargo'

cargo_env = environment()
cargo_env.set('CARGO_TARGET_DIR', cargo_target_dir)

is_mingw = host_machine.system() == 'windows' and cpp.get_id() == 'gcc'
is_msvc = cpp.get_argument_syntax() == 'msvc'
cargo_cmd = [
  cargo, 'build', '--release',
  '--manifest-path', project_source / 'Cargo.toml',
]

if is_mingw
  rust_target = 'x86_64-pc-windows-gnu'
  cargo_cmd += ['--target', rust_target]
  gpp = find_program('g++')
  cargo_env.set('CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER', gpp.full_path())
  ar = find_program('ar')
  cargo_env.set('CARGO_TARGET_X86_64_PC_WINDOWS_GNU_AR', ar.full_path())
  lib_path = cargo_target_dir / rust_target / 'release' / 'libvodozemac.a'
  rust_syslibs = [
    '-lbcrypt', '-ladvapi32', '-lstdc++', '-lkernel32', '-lntdll',
    '-luserenv', '-lws2_32', '-ldbghelp',
  ]
elif is_msvc
  lib_path = cargo_target_dir / 'release' / 'vodozemac.lib'
  rust_syslibs = []
else
  lib_path = cargo_target_dir / 'release' / 'libvodozemac.a'
  rust_syslibs = []
endif

vodozemac_build = custom_target(
  'vodozemac-cargo-build',
  output: 'vodozemac.stamp',
  command: cargo_cmd,
  env: cargo_env,
  build_by_default: true,
  install: false,
)

vodozemac_dep = declare_dependency(
  link_args: [lib_path] + rust_syslibs,
  include_directories: include_directories(cargo_target_dir / 'cxxbridge'),
  sources: vodozemac_build,
)

meson.override_dependency('vodozemac-cpp', vodozemac_dep)

if get_option('tests')
  subdir('tests')
endif
