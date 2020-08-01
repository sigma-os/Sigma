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

x86_64::svm::vcpu::vcpu(generic::virt::vspace* space): gpr_state({}) {
    ASSERT(space->type == generic::virt::virt_types::Svm);

    auto* svm_vspace = (svm::vspace*)space->ptr;
    npt = &svm_vspace->context;

    vmcb_phys = mm::pmm::alloc_block(); // Since sizeof(vmcb_t) == 0x1000
    if(!vmcb_phys){
        printf("[SVM]: Failed to allocate block for vcpu vmcb\n");
        return;
    }

    vmcb = (vmcb_t*)((uint64_t)vmcb_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    mm::vmm::kernel_vmm::get_instance().map_page((uint64_t)vmcb_phys, (uint64_t)vmcb, map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);   

    *vmcb = vmcb_t{}; // Clear vmcb

    // Setup the Nested Paging
    vmcb->np_enable = 1; // Enable Nested Paging
    vmcb->n_cr3 = (uint64_t)npt->get_paging_info() - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
    vmcb->g_pat = x86_64::pat::default_pat;

    vmcb->icept_exceptions |= (1 << 1) | (1 << 6) | (1 << 14) | (1 << 17) | (1 << 18);

    vmcb->icept_cr_writes |= (1 << 8); // Intercept cr8 writes since no AVIC

    vmcb->icept_vmrun = 1;
    vmcb->icept_vmmcall = 1;
    vmcb->icept_vmload = 1;
    vmcb->icept_vmsave = 1;

    vmcb->icept_intr = 1;
    vmcb->icept_nmi = 1;
    vmcb->icept_smi = 1;

    vmcb->icept_task_switch = 1;

    vmcb->icept_hlt = 1; // DEBUG: Intercept hlt
    vmcb->icept_cpuid = 1;
    vmcb->icept_rdpmc = 1;
    vmcb->icept_rdtsc = 1;
    vmcb->icept_invd = 1;
    vmcb->icept_stgi = 1;
    vmcb->icept_clgi = 1;
    vmcb->icept_skinit = 1;
    vmcb->icept_xsetbv = 1;
    vmcb->icept_wbinvd = 1;
    vmcb->icept_rdpru = 1;
    vmcb->icept_rsm = 1;

    vmcb->icept_efer_write = 1;

    vmcb->v_intr_masking = 1;


    msr_bitmap_phys = mm::pmm::alloc_n_blocks(svm::msr_bitmap_size);
    io_bitmap_phys = mm::pmm::alloc_n_blocks(svm::io_bitmap_size);

    msr_bitmap = (uint8_t*)((uint64_t)msr_bitmap_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    io_bitmap = (uint8_t*)((uint64_t)io_bitmap_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

    for(size_t i = 0; i < svm::msr_bitmap_size; i++){
        uint64_t phys = (uint64_t)msr_bitmap_phys + (i * mm::pmm::block_size);
        uint64_t virt = (uint64_t)msr_bitmap_phys + (i * mm::pmm::block_size) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
        mm::vmm::kernel_vmm::get_instance().map_page(phys, virt, map_page_flags_present | map_page_flags_writable);
    }

    for(size_t i = 0; i < svm::io_bitmap_size; i++){
        uint64_t phys = (uint64_t)io_bitmap_phys + (i * mm::pmm::block_size);
        uint64_t virt = (uint64_t)io_bitmap_phys + (i * mm::pmm::block_size) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
        mm::vmm::kernel_vmm::get_instance().map_page(phys, virt, map_page_flags_present | map_page_flags_writable);
    }

    vmcb->msrpm_base_phys = (uint64_t)msr_bitmap_phys;
    vmcb->iopm_base_phys = (uint64_t)io_bitmap_phys;

    vmcb->icept_msr = 1;
    //vmcb->icept_io = 1;

    memset(msr_bitmap, 0xFF, svm::msr_bitmap_size * mm::pmm::block_size);
    memset(io_bitmap, 0xFF, svm::io_bitmap_size * mm::pmm::block_size);


    vmcb->guest_asid = 1;
    vmcb->tlb_control = 1; // Flush all ASIDs every vmrun so we don't have to care about using different ASIDs

    auto init_selector = []() -> x86_64::svm::vmcb_t::segment {
        x86_64::svm::vmcb_t::segment seg{};
        seg.limit = 0xFFFF;
        seg.attrib = 0x93;
        return seg;
    };

    vmcb->ds = init_selector();
    vmcb->es = init_selector();
    vmcb->fs = init_selector();
    vmcb->gs = init_selector();
    vmcb->ss = init_selector();
    
    vmcb->idtr.limit = 0x3ff; // Limit for Real Mode IVT

    // Correct Boot Segment + RIP, Reset vector is Physical 0xFFFFFFF0
    vmcb->cs.selector = 0xF000;
    vmcb->cs.attrib = 0x9b;
    vmcb->cs.limit = 0xFFFF;
    vmcb->cs.base = 0xFFFF0000;
    vmcb->rip = 0x0000fff0;

    vmcb->efer = (1 << 12); // EFER.SVME is required to be set
    
    vmcb->rflags = (1 << 1); // Bit is reserved and should always be set
    vmcb->dr6 = 0xFFFF0FF0;
    vmcb->cr0 = (1 << 29) | (1 << 30) | (1 << 4); // set cr0.{NW, CD, ET}

    guest_simd = proc::simd::create_state();
    host_simd = proc::simd::create_state();
}

x86_64::svm::vcpu::~vcpu(){
    std::lock_guard lock{this->lock};
    proc::simd::destroy_state(guest_simd);
    proc::simd::destroy_state(host_simd);

    mm::pmm::free_block(vmcb_phys);
    for(size_t i = 0; i < svm::msr_bitmap_size; i++)
        mm::pmm::free_block((void*)((uintptr_t)msr_bitmap_phys + (i * mm::pmm::block_size)));
    for(size_t i = 0; i < svm::io_bitmap_size; i++)
        mm::pmm::free_block((void*)((uintptr_t)io_bitmap_phys + (i * mm::pmm::block_size)));
}

void x86_64::svm::vcpu::run(generic::virt::vexit* vexit){
    std::lock_guard lock{this->lock};
    while(true){
        asm("clgi");

        // {FS, GS, KernelGS}Base are not stored or reloaded by vmrun, so do it ourselves
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

        auto& cpu = *smp::cpu::get_current_cpu();
        
        // Reload TSS
        auto& tss_entry = cpu.gdt.get_entry_by_offset(cpu.tss_gdt_offset);
        // Make TSS available again
        tss_entry.ent &= ~(0x1Full << 40);
        tss_entry.ent |= (9ull << 40);
        cpu.tss.load(cpu.tss_gdt_offset);

        // Reload PAT
        x86_64::msr::write(x86_64::msr::ia32_pat, x86_64::pat::sigma_pat);
        asm("stgi");

        // TODO: Someway to see if the user wants to exit on hlt, for all these if(1)
        switch (vmcb->exitcode)
        {
        case 0x40 ... 0x5F: { // Exception[0, 31]
            uint8_t int_no = vmcb->exitcode - 0x40;
            printf("[SVM]: Interrupt, V: %x, gRIP: %x, gRSP: %x\n", int_no, vmcb->rip, vmcb->rsp);

            vexit->reason = generic::virt::vCtlExitReasonInterrupt;
            vexit->interrupt_number = int_no;
            return;
        }
        
        case 0x6e: // rdtsc
            vmcb->rip += 2;

            vexit->reason = generic::virt::vCtlExitReasonRdtsc;
            vexit->opcode[0] = 0x0F;
            vexit->opcode[1] = 0x31;
            vexit->opcode_length = 2;
            return;
        case 0x78: // hlt
            vmcb->rip++;

            vexit->reason = generic::virt::vCtlExitReasonHlt;
            vexit->opcode[0] = 0xF4;
            vexit->opcode_length = 1;
            return;
        case 0x7B: {// Port IO
            // TODO: Not done yet
            auto old_rip = vmcb->rip;
            printf("YEET: %x vs %x\n", vmcb->rip, vmcb->exitinfo2);
            vmcb->rip = vmcb->exitinfo2; // Exitinfo2 contains the next rip

            vexit->reason = (vmcb->exitinfo1 & (1 << 0)) ? (generic::virt::vCtlExitReasonPortRead) : (generic::virt::vCtlExitReasonPortWrite);

            vexit->port.port = (vmcb->exitinfo1 >> 16) & 0xFFFF;
                
            if(vmcb->exitinfo1 & (1 << 4))
                vexit->port.width = 8;
            else if(vmcb->exitinfo1 & (1 << 5))
                vexit->port.width = 16;
            else if(vmcb->exitinfo1 & (1 << 6))
                vexit->port.width = 32;
                
            vexit->port.repeated = (vmcb->exitinfo1 & (1 << 3));
            vexit->port.string = (vmcb->exitinfo1 & (1 << 2));

            vexit->opcode_length = vmcb->rip - old_rip;

            auto host_phys_rip = npt->get_phys(vmcb->cs.base + old_rip);
            if(host_phys_rip != ~1ull){
                mm::vmm::kernel_vmm::get_instance().map_page(host_phys_rip, host_phys_rip + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE, map_page_flags_present);

                for(int i = 0; i < vexit->opcode_length; i++)
                    vexit->opcode[i] = ((uint8_t*)host_phys_rip + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE)[i];

            } else {
                PANIC("Couldn't find gRIP mapping");
            }
            return;
        }
        case 0x81: // vmmcall
            vmcb->rip += 3;

            vexit->reason = generic::virt::vCtlExitReasonHypercall;
            vexit->opcode[0] = 0x0F;
            vexit->opcode[1] = 0x01;
            vexit->opcode[2] = 0xD9;
            vexit->opcode_length = 3;
            return;
        case 0x400: { // Nested Page Fault
            printf("[SVM] Nested Page Fault, Faulting address: %x ", vmcb->exitinfo2);

            if(!(vmcb->exitinfo1 & (1 << 0)))
                printf("Non-present; ");
            if(vmcb->exitinfo1 & (1 << 1))
                printf("Write; ");
            else
                printf("Read; ");
            if(vmcb->exitinfo1 & (1 << 3))
                printf("Reserved bits set; ");
            if(vmcb->exitinfo1 & (1 << 4))
                printf("Instruction fetch; ");
            if(vmcb->exitinfo1 & (1ull << 32))
                printf("Error during final physical translation; ");
            if(vmcb->exitinfo1 & (1ull << 33))
                printf("Error during guest page table translation; ");
            printf("\n");

            auto rip = vmcb->cs.base + vmcb->rip;

            auto host_phys_rip = npt->get_phys(rip);

            printf("      Address Page entry: %x\n", npt->get_entry(vmcb->exitinfo2));
            printf("      gRIP Page entry: %x\n", npt->get_entry(rip));
            printf("      gRIP: [gVirtual: %x, hPhys: %x]\n      Instruction bytes: \n      ", rip, host_phys_rip);

            if(host_phys_rip != ~1ull){
                mm::vmm::kernel_vmm::get_instance().map_page(host_phys_rip, host_phys_rip + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE, map_page_flags_present);

                for(int i = 0; i < 15; i++)
                    printf("%x ", *((uint8_t*)host_phys_rip + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + i));
                printf("\n");
            }


            /*printf("      Decode assist: fetched instruction bytes: %x\n      Instruction bytes: \n      ", vmcb->instruction_len);
            for(int i = 0; i < vmcb->instruction_len; i++)
                printf("%x ", vmcb->instruction_bytes[i]);
            printf("\n");*/

            // TODO: VEXIT

            asm("cli; hlt");
            break;
        }
        case (uint32_t)~0:
            printf("[SVM]: Invalid VMCB state\n");

            if(1){
                vexit->reason = generic::virt::vCtlExitReasonInvalidInternalState;
                return;
            }
            break;
        default:
            printf("[SVM] Unknown exitcode: %x\n", vmcb->exitcode);
            
            asm("cli; hlt");
            break;
        }
    }
}

void x86_64::svm::vcpu::get_regs(generic::virt::vregs* regs){
    std::lock_guard lock{this->lock};
    regs->rax = vmcb->rax;

    regs->rbx = gpr_state.rbx;
    regs->rcx = gpr_state.rcx;
    regs->rdx = gpr_state.rdx;
    regs->rsi = gpr_state.rsi;
    regs->rdi = gpr_state.rdi;
    regs->r8 = gpr_state.r8;
    regs->r9 = gpr_state.r9;
    regs->r10 = gpr_state.r10;
    regs->r11 = gpr_state.r11;
    regs->r12 = gpr_state.r12;
    regs->r13 = gpr_state.r13;
    regs->r14 = gpr_state.r14;
    regs->r15 = gpr_state.r15;

    regs->rsp = vmcb->rsp;
    regs->rbp = gpr_state.rbp;
    regs->rip = vmcb->rip;
    regs->rflags = vmcb->rflags;

    regs->cr0 = vmcb->cr0;
    regs->cr2 = vmcb->cr2;
    regs->cr3 = vmcb->cr3;
    regs->cr4 = vmcb->cr4;
    regs->cr8 = 0; // TODO: Save cr8

    regs->efer = vmcb->efer;

    regs->cs = vmcb->cs;
    regs->ds = vmcb->ds;
    regs->es = vmcb->es;
    regs->ss = vmcb->ss;
    regs->fs = vmcb->fs;
    regs->gs = vmcb->gs;
    regs->ldtr = vmcb->ldtr;
    regs->tr = vmcb->tr;

    regs->gdtr = vmcb->gdtr;
    regs->idtr = vmcb->idtr;
}

void x86_64::svm::vcpu::set_regs(generic::virt::vregs* regs){
    std::lock_guard lock{this->lock};
    vmcb->rax = regs->rax;

    gpr_state.rbx = regs->rbx;
    gpr_state.rcx = regs->rcx;
    gpr_state.rdx = regs->rdx;
    gpr_state.rsi = regs->rsi;
    gpr_state.rdi = regs->rdi;
    gpr_state.r8 = regs->r8;
    gpr_state.r9 = regs->r9;
    gpr_state.r10 = regs->r10;
    gpr_state.r11 = regs->r11;
    gpr_state.r12 = regs->r12;
    gpr_state.r13 = regs->r13;
    gpr_state.r14 = regs->r14;
    gpr_state.r15 = regs->r15;

    vmcb->rsp = regs->rsp;
    gpr_state.rbp = regs->rbp;
    vmcb->rip = regs->rip;
    vmcb->rflags = regs->rflags;

    vmcb->cr0 = regs->cr0;
    vmcb->cr2 = regs->cr2;
    vmcb->cr3 = regs->cr3;
    vmcb->cr4 = regs->cr4;
    // TODO: cr8

    vmcb->efer = regs->efer;

    vmcb->cs = svm::vmcb_t::segment{regs->cs};
    vmcb->ds = svm::vmcb_t::segment{regs->ds};
    vmcb->es = svm::vmcb_t::segment{regs->es};
    vmcb->ss = svm::vmcb_t::segment{regs->ss};
    vmcb->fs = svm::vmcb_t::segment{regs->fs};
    vmcb->gs = svm::vmcb_t::segment{regs->gs};

    vmcb->ldtr = svm::vmcb_t::segment{regs->ldtr};
    vmcb->tr = svm::vmcb_t::segment{regs->tr};
    vmcb->gdtr = svm::vmcb_t::segment{regs->gdtr};
    vmcb->idtr = svm::vmcb_t::segment{regs->idtr};
}

x86_64::svm::vspace::vspace(): context{x86_64::paging::context{}} {
	context.init();
}

x86_64::svm::vspace::~vspace(){
    std::lock_guard lock{this->lock};
	context.deinit();
}

void x86_64::svm::vspace::map(uint64_t host_phys, uint64_t guest_phys){
    std::lock_guard lock{this->lock};
	context.map_page(host_phys, guest_phys, map_page_flags_present | map_page_flags_writable | map_page_flags_user);
}
