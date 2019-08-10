project('libsigma', 'c')

libsigma_includes = include_directories('include')

libsigma_sources = files(
    'source/syscall.c',
    'source/klog.c',
    'source/thread.c')

c_args = ['-std=gnu18']

static_library('sigma', libsigma_sources, c_args: c_args, include_directories: libsigma_includes, pic: false, install: true)

libsigma_api_headers = files(
    'include/libsigma/klog.h',
    'include/libsigma/syscall.h',
    'include/libsigma/thread.h')

install_headers(libsigma_api_headers, subdir: 'libsigma')