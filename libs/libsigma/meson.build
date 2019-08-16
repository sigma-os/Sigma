project('libsigma', 'c')

headers_only = get_option('headers_only')
no_headers = get_option('mlibc_no_headers')

if no_headers

libsigma_includes = include_directories('include')

libsigma_sources = files(
    'source/syscall.c',
    'source/klog.c',
    'source/thread.c',
    'source/memory.c',
    'source/file.c')

c_args = ['-std=gnu18']

libsigma = static_library('sigma', libsigma_sources, c_args: c_args, include_directories: libsigma_includes, pic: false, install: true)

pkg = import('pkgconfig')
pkg.generate(libsigma)
endif

if headers_only

libsigma_api_headers = files(
    'include/libsigma/klog.h',
    'include/libsigma/syscall.h',
    'include/libsigma/thread.h',
    'include/libsigma/memory.h',
    'include/libsigma/file.h')

install_headers(libsigma_api_headers, subdir: 'libsigma')

endif