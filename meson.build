project('vodozemac-cpp', 'cpp', version: '0.1.0',
  default_options: [
    'cpp_std=c++23',
    'warning_level=3'
    ]
  )

cargo = find_program('cargo')
run_command(cargo, 'build', '--release', check: true)

if host_machine.system() == 'windows'
  lib_path = meson.current_source_dir() / 'target' / 'release' / 'vodozemac.lib'
else
  lib_path = meson.current_source_dir() / 'target' / 'release' / 'libvodozemac.a'
endif

vodozemac_dep = declare_dependency(
  link_args : [lib_path],
  include_directories : 'target' / 'cxxbridge'
)

meson.override_dependency('vodozemac-cpp', vodozemac_dep)