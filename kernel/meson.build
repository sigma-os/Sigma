
conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'include/Sigma/config.h.in', output: 'config.h', configuration: conf_data)

main_sources = files(
    'source/acpi/acpi.cpp',
    'source/acpi/madt.cpp',
    'source/acpi/laihost.cpp',
    'source/arch/x86_64/drivers/apic.cpp',
    'source/arch/x86_64/drivers/hpet.cpp',
    'source/arch/x86_64/drivers/pic.cpp',
    'source/arch/x86_64/drivers/pci.cpp',
    'source/arch/x86_64/gdt.cpp',
    'source/arch/x86_64/idt.cpp',
    'source/arch/x86_64/io.cpp',
    'source/arch/x86_64/msr.cpp',
    'source/arch/x86_64/paging.cpp',
    'source/arch/x86_64/vga.cpp',
    'source/arch/x86_64/misc/misc.cpp',
    'source/klibc/stdio.cpp',
    'source/klibc/stdlib.cpp',
    'source/klibc/string.cpp',
    'source/mm/hmm.cpp',
    'source/mm/pmm.cpp',
    'source/mm/vmm.cpp',
    'source/mm/alloc.cpp',
    'source/smp/smp.cpp',
    'source/smp/ipi.cpp',
    'source/smp/trampoline.S',
    'source/proc/initrd.cpp',
    'source/proc/ipc.cpp',
    'source/proc/process.cpp',
    'source/proc/elf.cpp',
    'source/proc/syscall.cpp',
    'source/cpp_support.cpp',
    'source/crti.S',
    'source/crtn.S',
    'source/kernel_main.cpp',
    'source/panic.cpp',
    'source/debug.cpp')

cpp_includes = include_directories('include')

nasm_sources = files(
    'source/arch/x86_64/misc/spinlock.asm',
    'source/arch/x86_64/interrupts.asm',
    'source/proc/process_low.asm',
    'source/smp/gs.asm',
    'source/kernel_early.asm',)

nasm = find_program('nasm')
nasm_gen = generator(nasm, output: '@BASENAME@.o', arguments: ['-f', 'elf64', '@INPUT@', '-o', '@OUTPUT@'])
nasm_objects = nasm_gen.process(nasm_sources)

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

extra_warning_flags = ['-Wreorder', '-Wno-unknown-pragmas', '-Wduplicated-cond', '-Wduplicated-branches',
                       '-Wlogical-op', '-Wnull-dereference', '-Werror']

flags_c_common = ['-m64', '-march=x86-64', '-fno-PIC', '-Wall', '-Wextra', '-ffreestanding', '-mno-red-zone',
                  '-nostdlib', '-mcmodel=kernel', '-fno-stack-protector', '-fno-omit-frame-pointer', '-g', '-O2',
                  '-mno-mmx', '-mno-sse', '-mno-sse2', '-mno-sse3', '-mno-sse4', '-mno-sse4.1', '-mno-sse4.2', '-mno-sse4a']

cpp_flags = []
cpp_flags += flags_c_common
cpp_flags += extra_warning_flags
cpp_flags += ['-std=c++17', '-fno-exceptions', '-fno-rtti', '-fuse-cxa-atexit']

c_flags = []
c_flags += flags_c_common
c_flags += ['-std=c18']

ld_script = meson.current_source_dir() + '/build/linker.ld'

ld_flags = ['-T', ld_script, '-nostdlib', '-mcmodel=kernel', '-fno-PIC', '-no-pie', '-Wl,--build-id=none', '-Wl,-z,max-page-size=0x1000,-n', '-lgcc']


add_global_arguments(c_flags, language: 'c')
add_global_link_arguments(ld_flags, language: 'c')
add_global_arguments(cpp_flags, language: 'cpp')
add_global_link_arguments(ld_flags, language: 'cpp')

lai = subproject('lai')
lai_dependency = lai.get_variable('dependency')

loader = subproject('multiboot_loader')
loader_dependency = loader.get_variable('dependency')
loader_extra_objects = loader.get_variable('extra_object_files')

deps = [lai_dependency, loader_dependency]

executable('sigma.bin', main_sources, nasm_objects, loader_extra_objects, dependencies: deps, include_directories: cpp_includes, link_depends: ld_script, objects: crt_files, install: true)
