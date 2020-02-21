project('libkbus', 'cpp')

kbus_includes = include_directories('include')

kbus_sources = files('source/kbus.cpp')

libsigma_dep = dependency('sigma')
kbus_deps = [libsigma_dep]

cpp_args = ['-std=c++17']

libkbus = shared_library('kbus', kbus_sources, dependencies: kbus_deps, cpp_args: cpp_args, include_directories: kbus_includes, install: true)

pkg = import('pkgconfig')
pkg.generate(libkbus)

kbus_api_headers = files('include/kbus/kbus.hpp')

install_headers(kbus_api_headers, subdir: 'kbus')