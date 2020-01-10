nvme_sources = files('source/main.cpp', 'source/io_controller.cpp', 'source/queue.cpp')
nvme_include_dirs = [default_include_dirs, include_directories('include')]
nvme_deps = [default_deps]

executable('nvme', nvme_sources, include_directories: nvme_include_dirs, dependencies: nvme_deps, install: true)