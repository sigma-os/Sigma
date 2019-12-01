project('ahci', 'cpp', version: '0.0.1', default_options : ['cpp_std=c++17'])

#include_dirs = include_directories('include')

source_files = files('main.cpp', 'ahci.cpp')        

libsigma_dep = dependency('sigma')
deps = [libsigma_dep]

add_project_arguments('-Wall', '-Wextra', '-Wno-unknown-pragmas', '-fstrict-volatile-bitfields', language: 'cpp')


executable('ahci', source_files, dependencies: deps, install: true)