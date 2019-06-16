project('Sigma OS', 'cpp', version: '0.1.0')

deps = []

crtbegin_obj_command = run_command(meson.get_compiler('cpp'), '-print-file-name=crtbegin.o')
if crtbegin_obj_command.returncode() != 0
    error('Could not find crtbegin.o')
endif
crtbegin_obj = crtbegin_obj_command.stdout().strip()

crtend_obj_command = run_command(meson.get_compiler('cpp'), '-print-file-name=crtend.o')
if crtend_obj_command.returncode() != 0
    error('Could not find crtend.o')
endif
crtend_obj = crtend_obj_command.stdout().strip()

crt_files = files(crtbegin_obj, 
                  crtend_obj)

main_sources = files(
    'source/acpi/acpi.cpp',
    'source/arch/x86_64/drivers/apic.cpp',
    'source/arch/x86_64/drivers/cmos.cpp',
    'source/arch/x86_64/drivers/mp.cpp',
    'source/arch/x86_64/drivers/pic.cpp',
    'source/arch/x86_64/drivers/pit.cpp',
    'source/arch/x86_64/gdt.cpp',
    'source/arch/x86_64/idt.cpp',
    'source/arch/x86_64/io.cpp',
    'source/arch/x86_64/msr.cpp',
    'source/arch/x86_64/paging.cpp',
    'source/arch/x86_64/vga.cpp',
    'source//early/_start.S',
    'source/klibc/stdio.cpp',
    'source/klibc/stdlib.cpp',
    'source/klibc/string.cpp',
    'source/mm/hmm.cpp',
    'source/mm/pmm.cpp',
    'source/mm/vmm.cpp',
    'source/mm/slab.cpp',
    'source/smp/smp.cpp',
    'source/smp/trampoline.S',
    'source/cpp_support.cpp',
    'source/crti.S',
    'source/crtn.S',
    'source/kernel_main.cpp',
    'source/multiboot.cpp',
    'source/panic.cpp')

cpp_includes = include_directories('include')

nasm_sources = files(
    'source/arch/x86_64/misc/spinlock.asm',
    'source/arch/x86_64/interrupts.asm',
    'source/early/multiboot.asm',
    'source/smp/gs.asm',
    'source/kernel_early.asm',)

nasm = find_program('nasm')
nasm_gen = generator(nasm, output: '@BASENAME@.o', arguments: ['-f', 'elf64', '@INPUT@', '-o', '@OUTPUT@'])
nasm_objects = nasm_gen.process(nasm_sources)

extra_warning_flags = ['-Wreorder']

flags_c_common = ['-m64', '-march=x86-64', '-fno-PIC', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
                  '-nostdlib', '-mcmodel=kernel', '-fno-stack-protector', '-g', '-O2',
                  '-mno-mmx', '-mno-sse', '-mno-sse2', '-mno-sse3', '-mno-sse4', '-mno-sse4.1', '-mno-sse4.2', '-mno-sse4a']

cpp_flags = []
cpp_flags += flags_c_common
cpp_flags += extra_warning_flags
cpp_flags += ['-std=c++17', '-mno-red-zone', '-fno-exceptions', '-fno-rtti', '-fuse-cxa-atexit']


ld_script = meson.current_source_dir() + '/build/linker.ld'

ld_flags = ['-T', ld_script, '-nostdlib', '-mcmodel=kernel', '-fno-PIC', '-no-pie', '-Wl,--build-id=none', '-Wl,-z,max-page-size=0x1000,-n', '-lgcc']

executable('sigma.bin', main_sources, nasm_objects, dependencies: deps, include_directories: cpp_includes, cpp_args: cpp_flags, link_args: ld_flags, link_depends: ld_script, objects: crt_files)