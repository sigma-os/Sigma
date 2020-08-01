#include <Sigma/proc/process.h>
#include <Sigma/types/queue.h>
#include <Sigma/arch/x86_64/intel/vt-d.hpp>
#include <Sigma/generic/device.h>

auto thread_list = types::linked_list<proc::process::thread>();
static uint64_t current_thread_list_offset = 0;

auto cpus = misc::lazy_initializer<types::linked_list<proc::process::managed_cpu>>();

proc::process::thread* kernel_thread;

proc::process::managed_cpu* proc::process::get_current_managed_cpu(){
	if(!cpus.is_initialized())
		return nullptr;
	
	auto current_apic_id = smp::cpu::get_current_cpu()->lapic_id;
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
		auto check_thread = [](proc::process::thread& t) -> bool {
			std::lock_guard guard{t.thread_lock};
			if(t.state == proc::process::thread_state::IDLE){
				return true;
			} else if(t.state == proc::process::thread_state::BLOCKED){
				if(t.event->has_triggered()){
					t.state = proc::process::thread_state::IDLE;
					return true;
				}
			}

			return false;
		};

		if(current == nullptr){
			// Just got called without any current thread, loop through all of them
			for(auto& entry : thread_list)
				if(check_thread(entry))
					return &entry;
			
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
			if(check_thread(entry))
				return &entry;
			
			end = thread_list.end();
			++it;
		}

		it = thread_list.begin();
		end = current_it;
		while(it != end){
			auto& entry = *it;
			if(check_thread(entry))
				return &entry;
			
			++it;
		}

		return nullptr;
	};

	if(current)
		std::lock_guard current_guard{current->thread_lock};

	if(scheduling_queue.length() >= 1){
		auto tid = scheduling_queue.pop();
		for(auto& entry : thread_list){
			if(entry.tid == tid)
				return &entry;
		}
		PANIC("Couldn't find thread referenced in queue");
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
	if(old_thread != nullptr)
		save_context(regs, old_thread); // This could be the first thread to run after an idle

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

	if(old_thread == nullptr || old_thread->context.cr3 != new_thread->context.cr3)
		new_thread->vmm.set();
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
		std::lock_guard guard{current_thread->thread_lock};
		if(current_thread->state == proc::process::thread_state::RUNNING)
			current_thread->state = proc::process::thread_state::IDLE;

		cpu->current_thread = nullptr; // Indicate to the scheduler that there is
									   // nothing left running on this cpu
	}


	uint64_t rsp = (uint64_t)smp::cpu::get_current_cpu()->idle_stack->top();
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

	proc::process::thread* old_thread = cpu->current_thread;

	proc::process::thread* new_thread = schedule(cpu->current_thread);
	if(!new_thread)
		idle_cpu(regs, cpu); 
	std::lock_guard new_guard{new_thread->thread_lock};

	if(old_thread && old_thread != new_thread)
		old_thread->thread_lock.lock();
	
	switch_context(regs, new_thread, old_thread);

	if(old_thread) {
		if(old_thread->state == proc::process::thread_state::RUNNING)
				old_thread->state = proc::process::thread_state::IDLE;
		if(old_thread != new_thread)
			old_thread->thread_lock.unlock();
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

static proc::process::thread* create_thread_int(proc::process::thread* thread, proc::process::thread_privilege_level privilege, proc::process::thread_state state) {
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{thread->thread_lock};
	thread->handle_catalogue = {};
	thread->stacks.reset();
	thread->context = {}; // Start with a clean slate, make sure no data leaks to the next thread
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
	thread->state = state;
	return thread;
}

void proc::process::make_kernel_thread(proc::process::thread* thread, void (*function)(void*), void* userptr){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{thread->thread_lock};

	mm::vmm::kernel_vmm::get_instance().clone_paging_info(thread->vmm);
	thread->context.cr3 = (thread->vmm.get_paging_info() - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

	auto kernel_thread_trampoline = +[](void (*f)(void*), void* userptr){
		f(userptr);

		printf("Left kernel thread?\n");
		get_current_thread()->set_state(proc::process::thread_state::SILENT);
		while(true)
			asm("hlt");
	};

	thread->context.rip = (uint64_t)kernel_thread_trampoline;
	thread->context.rsp = (uint64_t)thread->stacks.kernel_stack.top();
	thread->context.rdi = (uint64_t)function;
	thread->context.rsi = (uint64_t)userptr; // Bit hacky but assume sysv ABI
	thread->privilege = thread_privilege_level::KERNEL;
	thread->state = thread_state::IDLE;
}

proc::process::thread* proc::process::create_blocked_thread(proc::process::thread_privilege_level privilege){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{scheduler_mutex};
	for(auto& thread : thread_list){
		if(thread.state == proc::process::thread_state::DISABLED){
			// Found an empty thread in the list
			return create_thread_int(&thread, privilege, proc::process::thread_state::SILENT);
		}
	}

	auto* thread = thread_list.empty_entry();
	thread->tid = current_thread_list_offset++;
	return create_thread_int(thread, privilege, proc::process::thread_state::SILENT);
}

proc::process::thread* proc::process::thread_for_tid(tid_t tid){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{scheduler_mutex};
	for(auto& entry : thread_list){
		if(entry.tid == tid) return &entry;
	}

	return nullptr;
}

proc::process::thread* proc::process::get_current_thread(){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{scheduler_mutex}; // Some weird smp bug idfk
	
	return get_current_managed_cpu()->current_thread;
}

tid_t proc::process::get_current_tid(){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{scheduler_mutex}; // Some weird smp bug idfk

	return get_current_managed_cpu()->current_thread->tid;
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



void proc::process::kill(x86_64::idt::idt_registers* regs){
	mm::vmm::kernel_vmm::get_instance().set(); // We want nothing to do with this thread anymore
	proc::process::thread* thread = proc::process::get_current_thread();
	thread->thread_lock.lock();
	smp::cpu::get_current_cpu()->irq_lock.lock();
	scheduler_mutex.lock();


	thread->state = proc::process::thread_state::DISABLED;

	thread->stacks.reset();
	proc::simd::destroy_state(thread->context.simd_state);
	thread->context = proc::process::thread_context(); // Remove all traces from previous function
	thread->privilege = proc::process::thread_privilege_level::APPLICATION; // Lowest privilege
	for(auto& frame : thread->resources.frames) mm::pmm::free_block(reinterpret_cast<void*>(frame)); // Free frames
	thread->image = proc::process::thread_image();
	thread->vmm.deinit();
	thread->vmm.init();
	thread->thread_lock.unlock();

	auto* cpu = get_current_managed_cpu();
	cpu->current_thread = nullptr; // May this thread rest in peace

	scheduler_mutex.unlock();
	smp::cpu::get_current_cpu()->irq_lock.unlock();
	
	idle_cpu(regs, cpu);

	PANIC("idle_cpu returned, how is this happening");
}

tid_t proc::process::fork(x86_64::idt::idt_registers* regs){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	auto* parent = proc::process::get_current_thread();
	if(parent == nullptr) return 0;
	parent->thread_lock.lock();
	auto* child = proc::process::create_blocked_thread(parent->privilege);
	child->thread_lock.lock();

	save_context(regs, parent);

	parent->context.copy(child->context);
	child->image = parent->image;
	
	parent->vmm.fork_address_space(*child);

	child->context.cr3 = child->vmm.get_paging_info() - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;

	// Set return values
	child->context.rax = 0; // Child should get 0 as return value

	parent->thread_lock.unlock();
	child->thread_lock.unlock();
	child->wake();
	return child->tid;
}

void proc::process::yield(x86_64::idt::idt_registers* regs){
	timer_handler(regs, nullptr);
}

#pragma region proc::process::thread

void proc::process::thread::set_state(proc::process::thread_state new_state){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{this->thread_lock};
	this->state = new_state;
}

void proc::process::thread::block(generic::event* await, x86_64::idt::idt_registers* regs){
	smp::cpu::get_current_cpu()->irq_lock.lock();
	this->thread_lock.lock();
	this->event = await;
	this->state = proc::process::thread_state::BLOCKED;
	this->thread_lock.unlock();
	smp::cpu::get_current_cpu()->irq_lock.unlock();

	timer_handler(regs, nullptr); // Switch out of the thread
}

void proc::process::thread::wake(){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{this->thread_lock};
	this->state = proc::process::thread_state::IDLE;
}

bool proc::process::thread::is_blocked(){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{this->thread_lock};

	return this->state == proc::process::thread_state::BLOCKED;
}

void proc::process::thread::expand_thread_stack(size_t pages){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
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

	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
	std::lock_guard guard{this->thread_lock};

	this->context.fs = fs;
	x86_64::msr::write(x86_64::msr::fs_base, fs);
}

void* proc::process::thread::map_anonymous(size_t size, void *virt_base, void* phys_base, int prot, int flags){
	std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
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
	std::lock_guard guard{smp::cpu::get_current_cpu()->irq_lock};
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
		smp::cpu::get_current_cpu()->irq_lock.unlock();
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