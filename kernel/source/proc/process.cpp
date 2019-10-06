#include <Sigma/proc/process.h>

auto thread_list = types::linked_list<proc::process::thread>();
static uint64_t current_thread_list_offset = 0;

auto cpus = misc::lazy_initializer<types::vector<proc::process::managed_cpu>>();

proc::process::thread* kernel_thread;

proc::process::managed_cpu* proc::process::get_current_managed_cpu(){
    uint8_t current_apic_id = smp::cpu::get_current_cpu()->lapic_id;
    for(auto& entry : *cpus){
        if(entry.cpu.lapic_id == current_apic_id){
            return &entry; // Found this CPU
        }
    }

    return nullptr;
}

static proc::process::thread* schedule(proc::process::thread* current){
    if(current == nullptr){
        // Just got called without any current thread, loop through all of them
        for(auto& entry : thread_list){
            if(entry.state == proc::process::thread_state::IDLE){
                return &entry;
            }
        }
        return nullptr;
    }

    auto current_it = thread_list.get_iterator_for_item(current);
    if(current_it.entry == nullptr){
        debug_printf("[SCHEDULER]: Couldn't find current task in thread_list, task pointer: %x\n", current);
        return nullptr;
    }

    auto it = current_it;
    auto end = thread_list.end();
    while(it != end){
        auto& entry = *it;
        if(entry.state == proc::process::thread_state::IDLE){
            return &entry;
        }

        end = thread_list.end();
        ++it;
    }

    it = thread_list.begin();
    end = current_it;
    while(it != end){
        auto& entry = *it;
        if(entry.state == proc::process::thread_state::IDLE){
            return &entry;
        }

        ++it;
    }

    return nullptr;
}

static void save_context(x86_64::idt::idt_registers* regs, proc::process::thread* thread){
    thread->context.rax = regs->rax;
    thread->context.rbx = regs->rbx;
    thread->context.rcx = regs->rcx;
    thread->context.rdx = regs->rdx;
    thread->context.rdi = regs->rdi;
    thread->context.rsi = regs->rsi;
    thread->context.rbp = regs->rbp;
    thread->context.rsp = regs->rsp;
    thread->context.r8 = regs->r8;
    thread->context.r9 = regs->r9;
    thread->context.r10 = regs->r10;
    thread->context.r11 = regs->r11;
    thread->context.r12 = regs->r12;
    thread->context.r13 = regs->r13;
    thread->context.r14 = regs->r14;
    thread->context.r15 = regs->r15;

    thread->context.cs = regs->cs;
    thread->context.ss = regs->ss;
    thread->context.ds = regs->ds;
    thread->context.rip = regs->rip;
    thread->context.rflags = regs->rflags;

    thread->context.cr3 = reinterpret_cast<uint64_t>(x86_64::paging::get_current_info());
    proc::simd::save_state(thread);

    thread->context.fs = x86_64::msr::read(x86_64::msr::fs_base);
}

static void switch_context(x86_64::idt::idt_registers* regs, proc::process::thread* new_thread, proc::process::thread* old_thread){
    if(old_thread != nullptr) save_context(regs, old_thread); // This could be the first thread to run after an idle

    regs->rax = new_thread->context.rax;
    regs->rbx = new_thread->context.rbx;
    regs->rcx = new_thread->context.rcx;
    regs->rdx = new_thread->context.rdx;
    regs->rdi = new_thread->context.rdi;
    regs->rsi = new_thread->context.rsi;
    regs->rbp = new_thread->context.rbp;
    regs->rsp = new_thread->context.rsp;
    regs->r8 = new_thread->context.r8;
    regs->r9 = new_thread->context.r9;
    regs->r10 = new_thread->context.r10;
    regs->r11 = new_thread->context.r11;
    regs->r12 = new_thread->context.r12;
    regs->r13 = new_thread->context.r13;
    regs->r14 = new_thread->context.r14;
    regs->r15 = new_thread->context.r15;

    regs->cs = new_thread->context.cs;
    regs->ss = new_thread->context.ss;
    regs->ds = new_thread->context.ds;
    regs->rip = new_thread->context.rip;
    regs->rflags = new_thread->context.rflags;

    x86_64::msr::write(x86_64::msr::fs_base, new_thread->context.fs);

    if(old_thread == nullptr){
        x86_64::paging::set_current_info(reinterpret_cast<x86_64::paging::pml4*>(new_thread->context.cr3));
        proc::simd::restore_state(new_thread);
    } else {
        if(old_thread->context.cr3 != new_thread->context.cr3){
            x86_64::paging::set_current_info(reinterpret_cast<x86_64::paging::pml4*>(new_thread->context.cr3));
            proc::simd::restore_state(new_thread);
        }
    }
}

auto scheduler_mutex = x86_64::spinlock::mutex();

C_LINKAGE void proc_idle(uint64_t stack);

NORETURN_ATTRIBUTE
NOINLINE_ATTRIBUTE 
static void idle_cpu(x86_64::idt::idt_registers* regs, proc::process::managed_cpu* cpu) {
	auto* current_thread = cpu->current_thread;
	if(current_thread != nullptr) {			// Is this the second time in a row that the
											// cpu has idled a quantum?
		save_context(regs, current_thread); // Make sure the current thread can
											// be picked up at a later date
		switch(current_thread->state) {
			case proc::process::thread_state::RUNNING:
				current_thread->state = proc::process::thread_state::IDLE;
				break;

			default:
				break;
		}

		cpu->current_thread = nullptr; // Indicate to the scheduler that there is
									   // nothing left running on this cpu
	}


	uint64_t rsp = smp::cpu::get_current_cpu()->tss->rsp0;
	rsp = ALIGN_DOWN(rsp, 16); // Align stack for C code

	smp::cpu::get_current_cpu()->lapic.send_eoi();

	scheduler_mutex.release();

    mm::vmm::kernel_vmm::get_instance().set();
	proc_idle(rsp);

	while(true)
		; // proc_idle modifies the stack, it's dangerous, don't return ever
}

static void timer_handler(x86_64::idt::idt_registers* regs){
    scheduler_mutex.acquire();
    auto* cpu = proc::process::get_current_managed_cpu();
    if(cpu == nullptr){
        // This CPU is not managed abort
        debug_printf("[SCHEDULER]: Tried to schedule unmanaged CPU\n");
        scheduler_mutex.release();
        return;
    }
    proc::process::thread* new_thread = schedule(cpu->current_thread);
    if(new_thread == nullptr){
        idle_cpu(regs, cpu);
    }
    proc::process::thread* old_thread = cpu->current_thread;

    switch_context(regs, new_thread, old_thread);

    if(old_thread != nullptr) {
        switch (old_thread->state)
        {
        case proc::process::thread_state::RUNNING:
            old_thread->state = proc::process::thread_state::IDLE;
            break;
    
        default:
            break;
        }
    }
    
    cpu->current_thread = new_thread;
    cpu->current_thread->state = proc::process::thread_state::RUNNING;
    
    scheduler_mutex.release();
}

void proc::process::init_multitasking(acpi::madt& madt){
    auto cpu_list = types::linked_list<smp::cpu_entry>();
    madt.get_cpus(cpu_list);

    cpus.init();

    for(auto& entry : cpu_list) cpus->push_back(proc::process::managed_cpu(entry, false, nullptr));

    kernel_thread = thread_list.empty_entry();
    kernel_thread->tid = current_thread_list_offset++;
    kernel_thread->state = proc::process::thread_state::BLOCKED;

    x86_64::idt::register_interrupt_handler(proc::process::cpu_quantum_interrupt_vector, timer_handler, true);
    proc::simd::init_simd();
}

auto init_mutex = x86_64::spinlock::mutex();

void proc::process::init_cpu(){
    init_mutex.acquire();
    uint8_t current_apic_id = smp::cpu::get_current_cpu()->lapic_id;
    for(auto& entry : *cpus){
        if(entry.cpu.lapic_id == current_apic_id){
            // Found this CPU
            smp::cpu::get_current_cpu()->lapic.enable_timer(proc::process::cpu_quantum_interrupt_vector, proc::process::cpu_quantum, x86_64::apic::lapic_timer_modes::PERIODIC);
            entry.enabled = true;
            entry.current_thread = kernel_thread;
            proc::simd::init_ap_simd();
            init_mutex.release();
            return;
        }
    }
    init_mutex.release();
    printf("[MULTITASKING]: Tried to initialize cpu with apic_id: %x, that is not present in the tables\n", current_apic_id);
}

static proc::process::thread* create_thread_int(proc::process::thread* thread, uint64_t stack, void* rip,
												uint64_t cr3, proc::process::thread_privilege_level privilege,
												proc::process::thread_state state) {
	thread->ipc_manager.init(thread->tid);
	thread->context = proc::process::thread_context(); // Start with a clean slate, make sure
													   // no data leaks to the next thread
	thread->context.rip = reinterpret_cast<uint64_t>(rip);
	thread->context.cr3 = cr3;
	thread->context.rsp = stack;
	thread->state = state;
	thread->context.rflags = 0;
	thread->privilege = privilege;

	switch(thread->privilege) {
		case proc::process::thread_privilege_level::KERNEL:
			thread->context.cs = x86_64::gdt::kernel_code_selector;
			thread->context.ds = x86_64::gdt::kernel_data_selector;
			thread->context.ss = x86_64::gdt::kernel_data_selector;
			break;
		case proc::process::thread_privilege_level::DRIVER:
			thread->context.rflags |= ((1 << 12) | (1 << 13)); // Set IOPL
			FALLTHROUGH_ATTRIBUTE;
		case proc::process::thread_privilege_level::APPLICATION:
			thread->context.cs = x86_64::gdt::user_code_selector | 3;
			thread->context.ds = x86_64::gdt::user_data_selector | 3;
			thread->context.ss = x86_64::gdt::user_data_selector | 3; // Requested Privilege level 3
			break;
	}

	thread->context.rflags |= ((1 << 1) | (1 << 9)); // Bit 1 is reserved, should always be 1
													 // Bit 9 is IF, Interrupt flag, Force enable this
													 // so timer interrupts arrive
	return thread;
}

proc::process::thread* proc::process::create_thread(void* rip, uint64_t stack, uint64_t cr3, proc::process::thread_privilege_level privilege){
    for(auto& thread : thread_list){
        if(thread.state == proc::process::thread_state::DISABLED){
            // Found an empty thread in the list
            return create_thread_int(&thread, stack, rip, cr3, privilege, proc::process::thread_state::IDLE);
        }
    }

    auto* thread = thread_list.empty_entry();
    thread->tid = current_thread_list_offset++;
    return create_thread_int(thread, stack, rip, cr3, privilege, proc::process::thread_state::IDLE);
}

proc::process::thread* proc::process::create_kernel_thread(kernel_thread_function function){
    void* rip = reinterpret_cast<void*>(function);
    uint64_t cr3 = (mm::vmm::kernel_vmm::get_instance().get_paging_provider().get_paging_info() - KERNEL_VBASE);
    uint64_t stack = reinterpret_cast<uint64_t>(new uint8_t[0x1000]); // TODO: Improve this
    stack += 0x1000; // Remember stack grows downwards
    return proc::process::create_thread(rip, stack, cr3, proc::process::thread_privilege_level::KERNEL);
}

proc::process::thread* proc::process::create_blocked_thread(void* rip, uint64_t stack, uint64_t cr3, proc::process::thread_privilege_level privilege){
    for(auto& thread : thread_list){
        if(thread.state == proc::process::thread_state::DISABLED){
            // Found an empty thread in the list
            return create_thread_int(&thread, stack, rip, cr3, privilege, proc::process::thread_state::BLOCKED);
        }
    }

    auto* thread = thread_list.empty_entry();
    thread->tid = current_thread_list_offset++;
    return create_thread_int(thread, stack, rip, cr3, privilege, proc::process::thread_state::BLOCKED);
}

void proc::process::set_thread_state(proc::process::thread* thread, proc::process::thread_state new_state){
    thread->state = new_state;
}

proc::process::thread* proc::process::thread_for_tid(tid_t tid){
    for(auto& entry : thread_list){
        if(entry.tid == tid) return &entry;
    }

    return nullptr;
}

proc::process::thread* proc::process::get_current_thread(){
    return get_current_managed_cpu()->current_thread;
}

bool proc::process::receive_message(tid_t& origin, size_t& size, uint8_t* data){
    return get_current_thread()->ipc_manager.receive_message(origin, size, data);
}

size_t proc::process::get_message_size(){
    return get_current_thread()->ipc_manager.get_msg_size();
}

void proc::process::expand_thread_stack(proc::process::thread* thread, size_t pages){
    thread->thread_lock.acquire();
    for(uint64_t i = 0; i < pages; i++){
        void* phys = mm::pmm::alloc_block();
        if(phys == nullptr) PANIC("Couldn't allocate extra pages for thread stack");
        thread->resources.frames.push_back(reinterpret_cast<uint64_t>(phys));
        
        thread->image.stack_bottom -= mm::pmm::block_size;
        thread->vmm.map_page(reinterpret_cast<uint64_t>(phys), thread->image.stack_bottom, map_page_flags_present | map_page_flags_writable | map_page_flags_user | map_page_flags_no_execute);
    }
    thread->thread_lock.release();
}

void* proc::process::expand_thread_heap(proc::process::thread* thread, size_t pages){
    thread->thread_lock.acquire();
    uint64_t base = thread->image.heap_top;

    for(uint64_t i = 0; i < pages; i++){
        void* phys = mm::pmm::alloc_block();
        if(phys == nullptr) PANIC("Couldn't allocate extra pages for thread heap");
        thread->resources.frames.push_back(reinterpret_cast<uint64_t>(phys));

        thread->vmm.map_page(reinterpret_cast<uint64_t>(phys), thread->image.heap_top, map_page_flags_present | map_page_flags_writable | map_page_flags_user | map_page_flags_no_execute);
        thread->image.heap_top += mm::pmm::block_size;
    }
    thread->thread_lock.release();
    return reinterpret_cast<void*>(base);
}

tid_t proc::process::get_current_tid(){
    auto* cpu = get_current_managed_cpu();
    if(cpu == nullptr) return 0; // Kernel thread should be safe
    auto* thread = cpu->current_thread;
    if(thread == nullptr) return 0; // Same
    return thread->tid;
}

void proc::process::set_current_thread_fs(uint64_t fs){
    if(!misc::is_canonical(fs)) PANIC("Tried to set non canonical FS for thread");
    auto* thread = get_current_thread();
    if(thread == nullptr){
        PANIC("Tried to modify nullptr thread?");
        return;
    }

    thread->thread_lock.acquire();

    thread->context.fs = fs;
    x86_64::msr::write(x86_64::msr::fs_base, fs);
    
    thread->thread_lock.release();
}

void proc::process::kill(x86_64::idt::idt_registers* regs){
    mm::vmm::kernel_vmm::get_instance().set(); // We want nothing to do with this thread anymore
    proc::process::thread* thread = proc::process::get_current_thread();
    thread->thread_lock.acquire();


    thread->state = proc::process::thread_state::DISABLED;

    if(thread->context.simd_state) delete[] thread->context.simd_state;
    thread->context = proc::process::thread_context(); // Remove all traces from previous function
    thread->ipc_manager.deinit();
    thread->privilege = proc::process::thread_privilege_level::APPLICATION; // Lowest privilege
    for(auto& frame : thread->resources.frames) mm::pmm::free_block(reinterpret_cast<void*>(frame)); // Free frames
    thread->image = proc::process::thread_image();
    thread->vmm.deinit();
    thread->vmm.init();
    thread->thread_lock.release();

    auto* cpu = get_current_managed_cpu();
    cpu->current_thread = nullptr; // May this thread rest in peace

    idle_cpu(regs, cpu);

    PANIC("idle_cpu returned, how is this happening");
}

void proc::process::map_anonymous(proc::process::thread* thread, size_t size, void *addr, int prot, int flags){
    thread->thread_lock.acquire();

    if(!(flags & MAP_ANON)) PANIC("Only MAP_ANONYMOUS is defined");

    uint64_t map_flags = ((prot & PROT_READ) ? (map_page_flags_present) : 0) | \
                     ((prot & PROT_WRITE) ? (map_page_flags_writable) : 0) | \
                     (!(prot & PROT_EXEC) ? (map_page_flags_no_execute) : 0) | \
                     map_page_flags_user;

    size_t pages = misc::div_ceil(size, mm::pmm::block_size);
    uint64_t virt = reinterpret_cast<uint64_t>(addr);

    for(size_t i = 0; i < pages; i++, virt += mm::pmm::block_size){
        void* phys = mm::pmm::alloc_block();
        if(phys == nullptr) PANIC("Couldn't allocate pages for map_anonymous");
        thread->resources.frames.push_back(reinterpret_cast<uint64_t>(phys));

        thread->vmm.map_page(reinterpret_cast<uint64_t>(phys), virt, map_flags);
    }

    thread->thread_lock.release();
}