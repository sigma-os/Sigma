#include <Sigma/proc/process.h>
#include <Sigma/types/queue.h>
#include <Sigma/arch/x86_64/intel/vt-d.hpp>
#include <Sigma/generic/device.h>

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

types::queue<tid_t> scheduling_queue{};

static proc::process::thread* schedule(proc::process::thread* current){
    auto round_robin = [current]() -> proc::process::thread* {
        if(current == nullptr){
            // Just got called without any current thread, loop through all of them
            for(auto& entry : thread_list){
                if(entry.state == proc::process::thread_state::IDLE){
                    return &entry;
                } else if(entry.state == proc::process::thread_state::BLOCKED){
                    if(entry.event->has_triggered()){
                        entry.wake();
                        
                        return &entry;
                    }
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
            } else if(entry.state == proc::process::thread_state::BLOCKED){
                if(entry.event->has_triggered()){
                    entry.wake();
                    
                    return &entry;
                }
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
            } else if(entry.state == proc::process::thread_state::BLOCKED){
                if(entry.event->has_triggered()){
                    entry.wake();
                    
                    return &entry;
                }
            }

            ++it;
        }

        return nullptr;
    };

    if(scheduling_queue.length() >= 1){
        return proc::process::thread_for_tid(scheduling_queue.pop());
    } else {
        return round_robin();
    }
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

    if(!thread->context.simd_state)
        thread->context.simd_state = proc::simd::create_state();

    proc::simd::save_state(thread->context.simd_state);

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

    proc::simd::restore_state(new_thread->context.simd_state);

    if(old_thread == nullptr){
        x86_64::paging::set_current_info(&new_thread->vmm);
    } else {
        if(old_thread->context.cr3 != new_thread->context.cr3){
            x86_64::paging::set_current_info(&new_thread->vmm);
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

	scheduler_mutex.unlock();

    mm::vmm::kernel_vmm::get_instance().set();
	proc_idle(rsp);

	while(true)
		; // proc_idle modifies the stack, it's dangerous, don't return ever
}

static void timer_handler(x86_64::idt::idt_registers* regs, MAYBE_UNUSED_ATTRIBUTE void* userptr){
    scheduler_mutex.lock();
    auto* cpu = proc::process::get_current_managed_cpu();
    if(cpu == nullptr){
        // This CPU is not managed abort
        debug_printf("[SCHEDULER]: Tried to schedule unmanaged CPU\n");
        scheduler_mutex.unlock();
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
    
    scheduler_mutex.unlock();
}

void proc::process::init_multitasking(acpi::madt& madt){
    auto cpu_list = types::linked_list<smp::cpu_entry>();
    madt.get_cpus(cpu_list);

    cpus.init();

    for(auto& entry : cpu_list) cpus->push_back(proc::process::managed_cpu(entry, false, nullptr));

    kernel_thread = thread_list.empty_entry();
    kernel_thread->tid = current_thread_list_offset++;
    kernel_thread->state = proc::process::thread_state::SILENT;

    x86_64::idt::register_interrupt_handler({.vector = proc::process::cpu_quantum_interrupt_vector, .callback = timer_handler, .is_irq = true});
    proc::simd::init_simd();
}

auto init_mutex = x86_64::spinlock::mutex();

void proc::process::init_cpu(){
    std::lock_guard guard{init_mutex};
    uint8_t current_apic_id = smp::cpu::get_current_cpu()->lapic_id;
    for(auto& entry : *cpus){
        if(entry.cpu.lapic_id == current_apic_id){
            // Found this CPU
            smp::cpu::get_current_cpu()->lapic.enable_timer(proc::process::cpu_quantum_interrupt_vector, proc::process::cpu_quantum, x86_64::apic::lapic_timer_modes::PERIODIC);
            entry.enabled = true;
            entry.current_thread = kernel_thread;
            return;
        }
    }
    printf("[MULTITASKING]: Tried to initialize cpu with apic_id: %x, that is not present in the tables\n", current_apic_id);
}

static proc::process::thread* create_thread_int(proc::process::thread* thread, uint64_t stack, void* rip,
												uint64_t cr3, proc::process::thread_privilege_level privilege,
												proc::process::thread_state state) {
    thread->state = state;
	thread->ipc_manager.init(thread->tid);
	thread->context = proc::process::thread_context(); // Start with a clean slate, make sure
													   // no data leaks to the next thread
	thread->context.rip = reinterpret_cast<uint64_t>(rip);
	thread->context.cr3 = cr3;
	thread->context.rsp = stack;
	thread->context.rflags = ((1 << 1) | (1 << 9)); // Bit 1 is reserved, should always be 1
													// Bit 9 is IF, Interrupt flag, Force enable this
												    // so timer interrupts arrive
	thread->privilege = privilege;
    thread->context.simd_state = proc::simd::create_state();

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
    uint64_t cr3 = (mm::vmm::kernel_vmm::get_instance().get_paging_info() - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
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
    return create_thread_int(thread, stack, rip, cr3, privilege, proc::process::thread_state::SILENT);
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

void proc::process::block_thread(tid_t tid, generic::event* event, x86_64::idt::idt_registers* regs){
    thread_for_tid(tid)->block(event, regs);
}

void proc::process::wake_thread(tid_t tid){
    thread_for_tid(tid)->wake();
}

bool proc::process::is_blocked(tid_t tid){
    return proc::process::thread_for_tid(tid)->is_blocked();
}

bool proc::process::receive_message(tid_t* origin, size_t* size, uint8_t* data){
    return get_current_thread()->ipc_manager.receive_message(origin, size, data);
}

size_t proc::process::get_message_size(){
    return get_current_thread()->ipc_manager.get_msg_size();
}

tid_t proc::process::get_current_tid(){
    auto* cpu = get_current_managed_cpu();
    if(cpu == nullptr) return 0; // Kernel thread should be safe
    auto* thread = cpu->current_thread;
    if(thread == nullptr) return 0; // Same
    return thread->tid;
}

void proc::process::kill(x86_64::idt::idt_registers* regs){
    mm::vmm::kernel_vmm::get_instance().set(); // We want nothing to do with this thread anymore
    proc::process::thread* thread = proc::process::get_current_thread();
    thread->thread_lock.lock();


    thread->state = proc::process::thread_state::DISABLED;

    proc::simd::destroy_state(thread->context.simd_state);
    thread->context = proc::process::thread_context(); // Remove all traces from previous function
    thread->ipc_manager.deinit();
    thread->privilege = proc::process::thread_privilege_level::APPLICATION; // Lowest privilege
    for(auto& frame : thread->resources.frames) mm::pmm::free_block(reinterpret_cast<void*>(frame)); // Free frames
    thread->image = proc::process::thread_image();
    thread->vmm.deinit();
    thread->vmm.init();
    thread->thread_lock.unlock();

    auto* cpu = get_current_managed_cpu();
    cpu->current_thread = nullptr; // May this thread rest in peace

    idle_cpu(regs, cpu);

    PANIC("idle_cpu returned, how is this happening");
}

tid_t proc::process::fork(x86_64::idt::idt_registers* regs){
    auto* parent = proc::process::get_current_thread();
    if(parent == nullptr) return 0;
    parent->thread_lock.lock();
    auto* child = proc::process::create_blocked_thread(reinterpret_cast<void*>(regs->rip) \
                                        , regs->rsp, 0, parent->privilege);
    child->thread_lock.lock();

    save_context(regs, parent);

    parent->context.copy(child->context);
    child->image = parent->image;
    child->ipc_manager.init(child->tid);
    
    parent->vmm.fork_address_space(*child);

    child->context.cr3 = child->vmm.get_paging_info() - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;

    // Set return values
    child->context.rax = 0; // Child should get 0 as return value

    child->thread_lock.unlock();
    child->wake();
    parent->thread_lock.unlock();
    return child->tid;
}

void proc::process::yield(x86_64::idt::idt_registers* regs){
    timer_handler(regs, nullptr);
}

#pragma region proc::process::thread

void proc::process::thread::set_state(proc::process::thread_state new_state){
    std::lock_guard guard{this->thread_lock};
    this->state = new_state;
}

void proc::process::thread::block(generic::event* await, x86_64::idt::idt_registers* regs){
    this->thread_lock.lock();
    this->event = await;
    this->state = proc::process::thread_state::BLOCKED;
    this->thread_lock.unlock();
    timer_handler(regs, nullptr); // Switch out of the thread
}

void proc::process::thread::wake(){
    std::lock_guard guard{this->thread_lock};
    this->state = proc::process::thread_state::IDLE;
}

bool proc::process::thread::is_blocked(){
    std::lock_guard guard{this->thread_lock};

    return this->state == proc::process::thread_state::BLOCKED;
}

void proc::process::thread::expand_thread_stack(size_t pages){
    std::lock_guard guard{this->thread_lock};
    for(uint64_t i = 0; i < pages; i++){
        void* phys = mm::pmm::alloc_block();
        if(phys == nullptr)
            PANIC("Couldn't allocate extra pages for thread stack");
        this->resources.frames.push_back(reinterpret_cast<uint64_t>(phys));
        
        this->image.stack_bottom -= mm::pmm::block_size;
        this->vmm.map_page(reinterpret_cast<uint64_t>(phys), this->image.stack_bottom, map_page_flags_present | map_page_flags_writable | map_page_flags_user | map_page_flags_no_execute);
        memset_aligned_4k((void*)this->image.stack_bottom, 0);
    }
}

void proc::process::thread::set_fsbase(uint64_t fs){
    if(!misc::is_canonical(fs))
        PANIC("Tried to set non canonical FS for thread");

    std::lock_guard guard{this->thread_lock};

    this->context.fs = fs;
    x86_64::msr::write(x86_64::msr::fs_base, fs);
}

void* proc::process::thread::map_anonymous(size_t size, void *virt_base, void* phys_base, int prot, int flags){
    std::lock_guard guard{this->thread_lock};

    if(!virt_base)
        virt_base = reinterpret_cast<void*>(this->vmm.get_free_range(mmap_bottom, mmap_top, size));

    // If we couldn't find any just return
    if(!virt_base)
        return nullptr;

    (void)(flags);

    uint64_t map_flags = ((prot & PROT_READ) ? (map_page_flags_present) : 0) | \
                     ((prot & PROT_WRITE) ? (map_page_flags_writable) : 0) | \
                     (!(prot & PROT_EXEC) ? (map_page_flags_no_execute) : 0) | \
                     map_page_flags_user;

    size_t pages = misc::div_ceil(size, mm::pmm::block_size);
    uint64_t virt = reinterpret_cast<uint64_t>(virt_base);
    uint64_t phys = reinterpret_cast<uint64_t>(phys_base);

    bool allocate_phys = (phys_base == nullptr);

    for(size_t i = 0; i < pages; i++, virt += mm::pmm::block_size, phys += mm::pmm::block_size){
        if(allocate_phys){
            void* block = mm::pmm::alloc_block();
            if(block == nullptr) PANIC("Couldn't allocate pages for map_anonymous");
            this->resources.frames.push_back(reinterpret_cast<uint64_t>(block));
            this->vmm.map_page(reinterpret_cast<uint64_t>(block), virt, map_flags);
            memset_aligned_4k((void*)virt, 0);
        } else {
            // If requested to map a raw phys address also map it into the devices virtual space
            // TODO: Abstract for AMD IOMMU
            auto& iommu = x86_64::vt_d::get_global_iommu();
            auto& list = generic::device::get_device_list();
            if(iommu.is_active()){
                for(auto& device : list){
                    if(device.driver == this->tid){
                        auto& pci = *device.pci_contact.device;
                        auto& translation = iommu.get_translation(pci.seg, pci.bus, pci.device, pci.function);
                        translation.map(phys, phys, x86_64::sl_paging::mapSlPageRead | x86_64::sl_paging::mapSlPageWrite);
                    }
                }
            }
                

            this->vmm.map_page(phys, virt, map_flags);
        }
    }

    return virt_base;
}

bool proc::process::thread::get_phys_region(size_t size, int prot, int flags, phys_region* region){
    this->thread_lock.lock();
    size_t n_pages = misc::div_ceil(size, mm::pmm::block_size);
    void* phys_addr = mm::pmm::alloc_n_blocks(n_pages);
    
    if(phys_addr == nullptr){
        uintptr_t phys = (uintptr_t)phys_addr;
        for(size_t i = 0; i < n_pages; i++){
            uintptr_t cur_phys = phys + (mm::pmm::block_size * i);
            mm::pmm::free_block((void*)cur_phys);
        }
        this->thread_lock.unlock();
        return false;
    }

    uintptr_t phys = (uintptr_t)phys_addr;
    for(size_t i = 0; i < n_pages; i++){
        uintptr_t cur_phys = phys + (mm::pmm::block_size * i);
        this->resources.frames.push_back(cur_phys);
    } 
    
    this->thread_lock.unlock();
    void* virt_addr = this->map_anonymous(size, nullptr, phys_addr, prot, flags);
    if(virt_addr == nullptr)
        return false;

    memset(virt_addr, 0, size);
        
    region->physical_addr = (uint64_t)phys_addr;
    region->virtual_addr = (uint64_t)virt_addr;
    region->size = size;

    // If requested to map a raw phys address also map it into the devices virtual space
    // TODO: Abstract for AMD IOMMU
    auto& iommu = x86_64::vt_d::get_global_iommu();
    auto& list = generic::device::get_device_list();
    if(iommu.is_active()){
        for(auto& device : list){
            if(device.driver == this->tid){
                auto& pci = *device.pci_contact.device;
                auto& translation = iommu.get_translation(pci.seg, pci.bus, pci.device, pci.function);
                translation.map(phys, phys, x86_64::sl_paging::mapSlPageRead | x86_64::sl_paging::mapSlPageWrite);
            }
        }
    }

    return true;
}

#pragma endregion