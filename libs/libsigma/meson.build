project('libsigma', 'c')

headers_only = get_option('headers_only')
no_headers = get_option('no_headers')

if no_headers

libsigma_includes = include_directories('include')

libsigma_sources = files(
    'source/syscall.c',
    'source/sys.c',
    'source/virt.c')

c_args = ['-std=gnu18', '-fvisibility=hidden']

libsigma = static_library('sigma', libsigma_sources, c_args: c_args, include_directories: libsigma_includes, pic: false, install: true)

pkg = import('pkgconfig')
pkg.generate(libsigma)
endif

if headers_only

libsigma_api_headers = files(
    'include/libsigma/sys.h',
    'include/libsigma/syscall.h',
    'include/libsigma/virt.h')

install_headers(libsigma_api_headers, subdir: 'libsigma')

endif