catch2_dep = dependency(
  'catch2-with-main',
  version: '>=3.0.0',
  fallback: ['catch2', 'catch2_with_main_dep'],
  default_options: ['tests=false'],
)

vodozemac_tests = executable(
  'vodozemac_tests',
  'bindings_test.cpp',
  dependencies: [vodozemac_dep, catch2_dep],
  include_directories: include_directories('.'),
  cpp_args: ['-Wno-unused-parameter'],
  install: false,
)

test('vodozemac bindings', vodozemac_tests)
