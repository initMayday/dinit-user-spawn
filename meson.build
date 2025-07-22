project('dinit-user-spawn', 'cpp', version: '0.1', default_options: ['cpp_std=c++23'])

executable(
  'dinit-user-spawn',
  ['main.cpp'],
  include_directories: include_directories('.'),
  dependencies: [dependency('pam')],
  cpp_args: [],
  install: false,
)
