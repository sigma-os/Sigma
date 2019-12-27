project('sigma-drivers', 'cpp', version: '0.0.1', default_options : ['cpp_std=c++17'])

libsigma_dep = dependency('sigma')
deps = [libsigma_dep]

add_project_arguments('-Wall', '-Wextra', '-Wno-unknown-pragmas', '-fstrict-volatile-bitfields', language: 'cpp')

subdir('ata/')
subdir('ahci/')