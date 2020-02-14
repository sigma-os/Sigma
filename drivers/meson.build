project('sigma-drivers', 'cpp', version: '0.0.1', default_options : ['cpp_std=c++17'])

libsigma_dep = dependency('sigma')
default_deps = [libsigma_dep]

libdriver_include_dir = include_directories('.')
default_include_dirs = [libdriver_include_dir]

add_project_arguments('-Wall', '-Wextra', '-Wno-unknown-pragmas', '-fstrict-volatile-bitfields', language: 'cpp')

subdir('storage/')
subdir('video/')

subdir('usb/')