nvme_sources = files('main.cpp', 'nvme.cpp', 'nvme/queue.cpp')
nvme_include_dirs = [default_include_dirs]
nvme_deps = [default_deps]

executable('nvme', nvme_sources, include_directories: nvme_include_dirs, dependencies: nvme_deps, install: true)