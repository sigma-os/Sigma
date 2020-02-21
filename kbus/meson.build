project('kbus', 'cpp', version: '0.0.1', default_options : ['cpp_std=c++17'])

add_project_arguments('-Wall', '-Wextra', '-pedantic', '-Wno-unknown-pragmas', language: 'cpp')

executable('kbus', 'main.cpp', dependencies: dependency('sigma'), install: true)