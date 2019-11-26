project('ata', 'cpp', version: '0.0.1', default_options : ['cpp_std=c++17'])

#include_dirs = include_directories('include')

source_files = files(
                    'main.cpp',
                    'ata.cpp')

libsigma_dep = dependency('sigma')
deps = [libsigma_dep]

add_project_arguments('-Wall', '-Wextra', '-Wno-unknown-pragmas', language: 'cpp')


executable('ata', source_files, dependencies: deps, install: true)