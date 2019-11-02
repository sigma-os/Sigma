project('sigma-multiboot-loader', 'cpp')

loader_include = include_directories('include')

loader_nasm_sources = files(
    'source/load.asm',
    'source/multiboot.asm')

loader_nasm = find_program('nasm')
loader_nasm_gen = generator(loader_nasm, output: '@BASENAME@.o', arguments: ['-f', 'elf64', '-g', '-F', 'dwarf', '@INPUT@', '-o', '@OUTPUT@'])
extra_object_files = loader_nasm_gen.process(loader_nasm_sources)

loader_source_files = files(
    'source/common.cpp',
    'source/loader_main.cpp',
    'source/multiboot.cpp',
    'source/start_longmode.S')


loader_objects = [loader_source_files]

loader_library = static_library('multiboot_loader',
    loader_objects,
    include_directories: loader_include,
    pic: false)

dependency = declare_dependency(link_with: loader_library,
    include_directories: loader_include)