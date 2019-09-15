project('zeta', 'cpp', version: '0.0.1', default_options : ['cpp_std=c++17'])

include_dirs = include_directories('include')


source_files = files(
                    'source/main.cpp',
                    'source/vfs.cpp')

libsigma_dep = dependency('sigma')
deps = [libsigma_dep]


executable('zeta', source_files, include_directories: include_dirs, dependencies: deps, install: true)