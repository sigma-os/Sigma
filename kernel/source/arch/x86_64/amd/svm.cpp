#include <Sigma/arch/x86_64/amd/svm.hpp>
#include <Sigma/arch/x86_64/msr.h>
#include <Sigma/arch/x86_64/misc/misc.h>
#include <klibc/stdio.h>
#include <Sigma/arch/x86_64/cpu.h>
#include <Sigma/proc/process.h>

#include <Sigma/smp/cpu.h>

#include <Sigma/mm/pmm.h>
#include <Sigma/mm/vmm.h>

extern "C" void _vmrun(x86_64::svm::vm_gpr_state* gpr_state, void* vmcb);

bool x86_64::svm::init(){
    uint32_t a, b, c, d;
    if(!cpuid(0x80000001, a, b, c, d))
        return false; // CPUID leaf doesn't even exist

    if(!(c & (1 << 2)))
        return false; // Unsupported

    if(!cpuid(0x8000000A, a, b, c, d))
        PANIC("SVM support was detected but the CPUID leaf doesn't exist");
    
    // See SVM enable algorithm AMD64 spec Volume 2 Paragraph 15.4
    if(x86_64::msr::read(vm_cr) & (1 << 4)){
        if(d & (1 << 2))
            debug_printf("[SVM]: Disabled with key\n");
        else
            debug_printf("[SVM]: Disabled in BIOS\n");
        return false;
    }

    if(!(d & (1 << 0))){
        debug_printf("[SVM]: Nested Paging is required\n");
        return false;
    }
    
    #ifdef DEBUG
    debug_printf("[SVM]: Revision: %d, Number of ASIDs: %d, Features: ", a & 0xFF, b);
    if(d & (1 << 0))
        debug_printf("Nested paging; ");
    if(d & (1 << 1))
        debug_printf("Last branch virt; ");
    if(d & (1 << 2))
        debug_printf("SVM lock; ");
    if(d & (1 << 3))
        debug_printf("NRIP save; ");
    if(d & (1 << 4))
        debug_printf("TSC rate control; ");
    if(d & (1 << 5))
        debug_printf("VMCB clean bits; ");
    if(d & (1 << 6))
        debug_printf("Flush by ASID; ");
    if(d & (1 << 7))
        debug_printf("Decode assist; ");
    if(d & (1 << 10))
        debug_printf("Pause filter; ");
    if(d & (1 << 12))
        debug_printf("Pause filter threshold; ");
    if(d & (1 << 13))
        debug_printf("AVIC; ");
    if(d & (1 << 15))
        debug_printf("VM{SAVE, LOAD} virt; ");
    if(d & (1 << 16))
        debug_printf("GIF virt; ");
    debug_printf("\n");
    #endif

    x86_64::msr::write(x86_64::msr::ia32_efer, x86_64::msr::read(x86_64::msr::ia32_efer) | (1 << 12));

    // SVM needs a 4KiB host area
    x86_64::msr::write(vm_hsave_pa, (uint64_t)mm::pmm::alloc_block());

	smp::cpu::get_current_cpu()->features.svm = 1;

    return true;
}

x86_64::svm::vcpu::vcpu(virt::vspace* space): gpr_state({}) {
    ASSERT(space->type == virt::virt_types::Svm);

    auto* svm_vspace = (svm::vspace*)space->ptr;
    npt = &svm_vspace->context;

    vmcb_phys = mm::pmm::alloc_block(); // Since sizeof(vmcb_t) == 0x1000
    if(!vmcb_phys){
        printf("[SVM]: Failed to allocate block for vcpu vmcb\n");
        return;
    }

    vmcb = (vmcb_t*)((uint64_t)vmcb_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    mm::vmm::kernel_vmm::get_instance().map_page((uint64_t)vmcb_phys, (uint64_t)vmcb, map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);   

    *vmcb = {}; // Clear vmcb

    // Setup the Nested Paging
    vmcb->np_enable = 1; // Enable Nested Paging
    vmcb->n_cr3 = (uint64_t)npt->get_paging_info() - KERNEL_VBASE;
    vmcb->g_pat = x86_64::pat::pat;

    vmcb->icept_vmrun = 1;
    vmcb->icept_hlt = 1; // DEBUG: Intercept hlt
    vmcb->icept_rdtsc = 1;

    vmcb->guest_asid = 1; // TODO: tf is an ASID?

    auto init_selector = []() -> x86_64::svm::vmcb_t::selector {
        return {.selector = 0, .attrib = ((1 << 1) | (1 << 4) | (1 << 7)), .limit = 0xFFFF, .base = 0};
    };

    vmcb->ds = init_selector();
    vmcb->es = init_selector();
    vmcb->fs = init_selector();
    vmcb->gs = init_selector();
    vmcb->ss = init_selector();;
    vmcb->gdtr.limit = 0xFFFF;
    vmcb->idtr.limit = 0xFFFF;
    vmcb->ldtr.limit = 0xFFFF;
    vmcb->tr.limit = 0xFFFF;

    vmcb->cs = {};
    vmcb->cs.attrib = (1 << 1) | (1 << 3) | (1 << 4) | (1 << 7);
    vmcb->cs.limit = 0xFFFF;

    /* Correct Boot Segment + RIP
    vmcb->cs.selector = 0xF000;
    vmcb->cs.base = 0xFFFF0000;
    vmcb->cs.limit = 0xFFFF;

    vmcb->rip = 0x0000fff0;
    */

    // Start at 0 to test
    vmcb->rip = 0x1000; // Start
    vmcb->efer = (1 << 12); // set svme
    
    vmcb->rflags = (1 << 1); // Bit is reserved and should always be set

    guest_simd = proc::simd::create_state();
    host_simd = proc::simd::create_state();
}

x86_64::svm::vcpu::~vcpu(){
    proc::simd::destroy_state(guest_simd);
    proc::simd::destroy_state(host_simd);

    mm::pmm::free_block(vmcb_phys);
}

void x86_64::svm::vcpu::run(virt::vexit* vexit){
    while(true){
        asm("clgi");
        host_state.fs_base = x86_64::msr::read(x86_64::msr::fs_base);
        host_state.gs_base = x86_64::msr::read(x86_64::msr::gs_base);
        host_state.gs_kernel_base = x86_64::msr::read(x86_64::msr::kernelgs_base);

        proc::simd::save_state(host_simd);
        proc::simd::restore_state(guest_simd);
        
        _vmrun(&gpr_state, vmcb_phys);
        
        proc::simd::save_state(guest_simd);
        proc::simd::restore_state(host_simd); 

        x86_64::msr::write(x86_64::msr::fs_base, host_state.fs_base);
        x86_64::msr::write(x86_64::msr::gs_base, host_state.gs_base);
        x86_64::msr::write(x86_64::msr::kernelgs_base, host_state.gs_kernel_base);

        auto* cpu = smp::cpu::get_current_cpu();
        
        auto& tss_entry = cpu->gdt->get_entry_by_offset(cpu->tss_gdt_offset);
        tss_entry.ent &= ~(0x1Full << 40);
        tss_entry.ent |= (9ull << 40);
        cpu->tss->load(cpu->tss_gdt_offset);

        x86_64::msr::write(x86_64::msr::ia32_pat, x86_64::pat::pat);
        asm("stgi");

        // TODO: Someway to see if the user wants to exit on hlt, for all these if(1)
        switch (vmcb->exitcode)
        {
        case 0x400:
            printf("[SVM] Nested Page Fault, Faulting address: %x ", vmcb->exitinfo2, vmcb->exitinfo1);

            if(!(vmcb->exitinfo1 & (1 << 0)))
                printf("Non-present; ");
            if(vmcb->exitinfo1 & (1 << 1))
                printf("Write; ");
            else
                printf("Read; ");
            if(vmcb->exitinfo1 & (1 << 4))
                printf("Reserved bits set; ");
            if(vmcb->exitinfo1 & (1 << 5))
                printf("Code access; ");
            if(vmcb->exitinfo1 & (1ull << 32))
                printf("Error during final physical translation; ");
            if(vmcb->exitinfo1 & (1ull << 33))
                printf("Error during guest page table translation; ");
            printf("\n");

            printf("      RIP: %x\n", vmcb->rip);

            // TODO: VEXIT

            asm("cli; hlt");
            break;
        case 0x6e:
            vmcb->rip += 2;

            if(1){
                vexit->reason = virt::vCtlExitReasonRdtsc;
                vexit->opcode[0] = 0x0F;
                vexit->opcode[1] = 0x31;
                vexit->opcode_length = 2;
                return;
            }
            break;
        case 0x78:
            vmcb->rip++;

            if(1){
                vexit->reason = virt::vCtlExitReasonHlt;
                vexit->opcode[0] = 0xF4;
                vexit->opcode_length = 1;
                return;
            }
            break;
        case -1:
            printf("[SVM]: Invalid VMCB state\n");

            if(1){
                vexit->reason = virt::vCtlExitReasonInvalidInternalState;
                return;
            }
            break;
        default:
            printf("[SVM] Unknown exitcode: %x\n", vmcb->exitcode);
            
            return;
        }
    }
}

x86_64::svm::vspace::vspace(): context{x86_64::paging::context{}} {
	context.init();
}

x86_64::svm::vspace::~vspace(){
	context.deinit();
}

void x86_64::svm::vspace::map(uint64_t host_phys, uint64_t guest_phys){
	context.map_page(host_phys, guest_phys, map_page_flags_present | map_page_flags_writable | map_page_flags_user);
}
