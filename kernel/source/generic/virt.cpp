#include <Sigma/generic/virt.hpp>
#include <Sigma/arch/x86_64/paging.h>
#include <Sigma/arch/x86_64/amd/svm.hpp>
#include <Sigma/smp/cpu.h>
#include <Sigma/generic/user_handle.hpp>
#include <Sigma/proc/process.h>

virt::virt_types get_type(){
	auto* cpu = smp::cpu::get_current_cpu();

	if(cpu->features.svm)
		return virt::virt_types::Svm;
	else
		return virt::virt_types::None;
}

virt::vspace::vspace(){
	type = get_type();
	switch(type){
		case virt_types::Svm: {
			auto* svm_vspace = new x86_64::svm::vspace{};
			ptr = (void*)svm_vspace;
			break;
		}
		case virt_types::None:
			printf("[VIRT]: No virtualization available\n");
			return;
	}
}

virt::vspace::~vspace(){
	switch(type){
		case virt_types::Svm: {
			if(ptr){
				auto* svm_vspace = (x86_64::svm::vspace*)ptr;
				delete svm_vspace;
			}
			break;
		}
		case virt_types::None:
			printf("[VIRT]: No virtualization available\n");
			return;
	}
}

void virt::vspace::map(uint64_t guest_phys, uint64_t host_phys){
	switch(type){
		case virt_types::Svm: {
			auto* svm_vspace = (x86_64::svm::vspace*)ptr;

			svm_vspace->map(host_phys, guest_phys);
			return;
		}
		case virt_types::None:
			printf("[VIRT]: No virtualization available\n");
			return;
	}
}


virt::vcpu::vcpu(vspace* space){
	type = get_type();
	switch(type){
		case virt_types::Svm: {
			auto* svm_vcpu = new x86_64::svm::vcpu{space};
			ptr = (void*)svm_vcpu;
			break;
		}
		case virt_types::None:
			printf("[VIRT]: No virtualization available\n");
			return;
	}
}

virt::vcpu::~vcpu(){
	switch(type){
		case virt_types::Svm: {
			if(ptr){
				auto* svm_vcpu = (x86_64::svm::vcpu*)ptr;

				delete svm_vcpu;
			}
			break;
		}
		case virt_types::None:
			printf("[VIRT]: No virtualization available\n");
			return;
	}
}

void virt::vcpu::run(virt::vexit* vexit){
	switch(type){
		case virt_types::Svm: {
			auto* svm_vcpu = (x86_64::svm::vcpu*)ptr;

			svm_vcpu->run(vexit);
			return;
		}
		case virt_types::None:
			printf("[VIRT]: No virtualization available\n");
			return;
	}
}

uint64_t virt::vctl(uint64_t cmd, MAYBE_UNUSED_ATTRIBUTE uint64_t arg1, MAYBE_UNUSED_ATTRIBUTE uint64_t arg2, MAYBE_UNUSED_ATTRIBUTE uint64_t arg3, MAYBE_UNUSED_ATTRIBUTE uint64_t arg4){
	switch(cmd){
		case vCtlCreateVcpu: {
			auto* thread = proc::process::get_current_thread();
			auto* vspace = thread->handle_catalogue.get<handles::vspace_handle>(arg1);
			auto vcpu_handle = thread->handle_catalogue.push(new handles::vcpu_handle{&vspace->space});
			#ifdef LOG_SYSCALLS
			printf("[VIRT]: vCtlCreateVcpu: vcpu: %d, vspace: %d\n", vcpu_handle, arg1);
			#endif
			return vcpu_handle;
		}
		case vCtlRunVcpu: {
			auto* vcpu = proc::process::get_current_thread()->handle_catalogue.get<handles::vcpu_handle>(arg1);
			vcpu->cpu.run((virt::vexit*)arg2);
			#ifdef LOG_SYSCALLS
			printf("[VIRT]: vCtlRunVcpu handle: %d, vexit: %x\n", arg1, ((virt::vexit*)arg2)->opcode[0]);
			#endif
			return 0;
		}
		case vCtlCreateVspace: {
			#ifdef LOG_SYSCALLS
			printf("[VIRT]: vCtlCreateVspace\n");
			#endif
			return proc::process::get_current_thread()->handle_catalogue.push(new handles::vspace_handle{});
		}
		case vCtlMapVspace: {
			#ifdef LOG_SYSCALLS
			printf("[VIRT]: vCtlMapVspace vspace: %d, guest_base: %x, host_base: %x, size: %x\n", arg1, arg2, arg3, arg4);
			#endif
			auto* thread = proc::process::get_current_thread();
			auto* vspace = thread->handle_catalogue.get<handles::vspace_handle>(arg1);

			uint64_t guest_phys_base = arg2;
			uint64_t host_virt_base = arg3;
			uint64_t size = arg4;

			for(uint64_t i = 0; i < misc::div_ceil(size, mm::pmm::block_size); i++){
				uint64_t host_phys = thread->vmm.get_phys(host_virt_base + (i * mm::pmm::block_size));
				uint64_t guest_phys = guest_phys_base + (i * mm::pmm::block_size);

				vspace->space.map(guest_phys, host_phys);
			}

			return 0;
		}
		default:
			printf("[VIRT]: Unknown vctl command: %x", cmd);
			break;
	}

	return -1;
}
