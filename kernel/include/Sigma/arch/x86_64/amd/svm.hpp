#ifndef SIGMA_KERNEL_ARCH_X86_64_AMD_SVM
#define SIGMA_KERNEL_ARCH_X86_64_AMD_SVM

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/paging.h>

#include <Sigma/generic/virt.hpp>

#include <klibcxx/mutex.hpp>

namespace x86_64::svm
{
	struct PACKED_ATTRIBUTE vmcb_t {
		vmcb_t() = default;

		struct {
			uint32_t icept_cr_reads : 16;
			uint32_t icept_cr_writes : 16;
		};
		struct {
			uint32_t icept_r_reads : 16;
			uint32_t icept_dr_writes : 16;
		};
		uint32_t icept_exceptions;
		struct {
			uint32_t icept_intr : 1;
			uint32_t icept_nmi : 1;
			uint32_t icept_smi : 1;
			uint32_t icept_init : 1;
			uint32_t icept_vintr : 1;
			uint32_t icept_cr0_writes : 1;
			uint32_t icept_idtr_reads : 1;
			uint32_t icept_gdtr_reads : 1;
			uint32_t icept_ldtr_reads : 1;
			uint32_t icept_tr_reads : 1;
			uint32_t icept_idtr_writes : 1;
			uint32_t icept_gdtr_writes : 1;
			uint32_t icept_ldtr_writes : 1;
			uint32_t icept_tr_writes : 1;
			uint32_t icept_rdtsc : 1;
			uint32_t icept_rdpmc : 1;
			uint32_t icept_pushf : 1;
			uint32_t icept_popf : 1;
			uint32_t icept_cpuid : 1;
			uint32_t icept_rsm : 1;
			uint32_t icept_iret : 1;
			uint32_t icept_int : 1;
			uint32_t icept_invd : 1;
			uint32_t icept_pause : 1;
			uint32_t icept_hlt : 1;
			uint32_t icept_invlpg : 1;
			uint32_t icept_invlpga : 1;
			uint32_t icept_io : 1;
			uint32_t icept_msr : 1;
			uint32_t icept_task_switch : 1;
			uint32_t ferr_freeze : 1;
			uint32_t icept_shutdown : 1;
		};
		struct {
			uint32_t icept_vmrun : 1;
			uint32_t icept_vmmcall : 1;
			uint32_t icept_vmload : 1;
			uint32_t icept_vmsave : 1;
			uint32_t icept_stgi : 1;
			uint32_t icept_clgi : 1;
			uint32_t icept_skinit : 1;
			uint32_t icept_rdtscp : 1;
			uint32_t icept_icebp : 1;
			uint32_t icept_wbinvd : 1;
			uint32_t icept_monitor : 1;
			uint32_t icept_mwait_unconditional : 1;
			uint32_t icept_mwait_if_armed : 1;
			uint32_t icept_xsetbv : 1;
			uint32_t icept_rdpru : 1;
			uint32_t icept_efer_write : 1;
			uint32_t icept_cr_writes_after_finish : 16;
		};
		uint8_t reserved_1[0x28];
		uint16_t pause_filter_threshold;
		uint16_t pause_filter_count;
		uint64_t iopm_base_phys;
		uint64_t msrpm_base_phys;
		uint64_t tsc_offset;
		struct {
			uint64_t guest_asid : 32;
			uint64_t tlb_control : 8;
			uint64_t reserved_2 : 24;
		};
		struct {
			uint64_t v_tpr : 8;
			uint64_t v_irq : 1;
			uint64_t reserved_3 : 7;
			uint64_t v_intr_priority : 4;
			uint64_t v_ignore_tpr : 1;
			uint64_t reserved_4 : 3;
			uint64_t v_intr_masking : 1;
			uint64_t reserved_5 : 7;
			uint64_t v_intr_vector : 8;
			uint64_t reserved_6 : 24;
		};
		struct {
			uint64_t irq_shadow : 1;
			uint64_t reserved_7 : 63;
		};
		uint64_t exitcode;
		uint64_t exitinfo1;
		uint64_t exitinfo2;
		uint64_t exitintinfo;
		struct {
			uint64_t np_enable : 1;
			uint64_t reserved_8 : 63;
		};
		uint8_t reserved_9[16];
		uint64_t event_inject;
		uint64_t n_cr3;
		struct {
			uint64_t lbr_enable : 1;
			uint64_t reserved_10 : 63;
		};
		uint32_t vmcb_clean;
		uint32_t reserved_11;
		uint64_t next_rip;
		uint8_t instruction_len;
		uint8_t instruction_bytes[15];
		uint8_t reserved_12[0x320];

		// Start of State Save Area
		struct PACKED_ATTRIBUTE segment {
			segment() = default;
			segment(generic::virt::vregs::selector in): selector{in.selector}, attrib{in.attrib}, limit{in.limit}, base{in.base} {}
			segment(generic::virt::vregs::dtable in): limit{in.limit}, base{in.base} {}

			uint16_t selector;
			uint16_t attrib;
			uint32_t limit;
			uint64_t base;

			operator generic::virt::vregs::selector(){
				return generic::virt::vregs::selector{.selector = selector, .attrib = attrib, .limit = limit, .base = base};
			}

			operator generic::virt::vregs::dtable(){
				return generic::virt::vregs::dtable{.base = base, .limit = (uint16_t)limit};
			}
		};

		segment es;
		segment cs;
		segment ss;
		segment ds;
		segment fs;
		segment gs;

		segment gdtr;
		segment ldtr;
		segment idtr;
		segment tr;

		uint8_t reserved_14[0x2B];
		uint8_t cpl;
		uint32_t reserved_15;
		uint64_t efer;
		uint8_t reserved_16[0x70];
		uint64_t cr4;
		uint64_t cr3;
		uint64_t cr0;
		uint64_t dr7;
		uint64_t dr6;
		uint64_t rflags;
		uint64_t rip;
		uint8_t reserved_17[0x58];
		uint64_t rsp;
		uint8_t reserved_18[0x18];
		uint64_t rax;
		uint64_t star;
		uint64_t lstar;
		uint64_t cstar;
		uint64_t sfmask;
		uint64_t kernel_gs_base;
		uint64_t sysenter_cs;
		uint64_t sysenter_esp;
		uint64_t sysenter_eip;
		uint64_t cr2;
		uint8_t reserved_19[0x20];
		uint64_t g_pat;
		uint64_t debug_control;
		uint64_t br_from;
		uint64_t br_to;
		uint64_t int_from;
		uint64_t int_to;
		uint8_t reserved_20[0x968];
	};
	static_assert(sizeof(vmcb_t) == 0x1000);

	struct vm_gpr_state {
		// Accessed from assembly, do not change offsets without changing svm_low.asm
		uint64_t rbx;
		uint64_t rcx;
		uint64_t rdx;
		uint64_t rsi;
		uint64_t rdi;
		uint64_t rbp;
		uint64_t r8;
		uint64_t r9;
		uint64_t r10;
		uint64_t r11;
		uint64_t r12;
		uint64_t r13;
		uint64_t r14;
		uint64_t r15;
	};

	struct vm_host_state {
		uint64_t fs_base, gs_base, gs_kernel_base;
	};

	constexpr uint32_t vm_cr = 0xC0010114;
	constexpr uint32_t vm_hsave_pa = 0xC0010117;

	bool init();

	constexpr size_t msr_bitmap_size = 2; // 2 Pages
	constexpr size_t io_bitmap_size = (UINT16_MAX / 8) / 0x1000;

	struct vcpu {
		vcpu(generic::virt::vspace* space);
		~vcpu();

		void run(generic::virt::vexit* vexit);
		void get_regs(generic::virt::vregs* regs);
		void set_regs(generic::virt::vregs* regs);

		private:
		std::mutex lock;

		void* vmcb_phys;
		vmcb_t* vmcb;

		vm_gpr_state gpr_state;
		vm_host_state host_state;

		uint8_t* msr_bitmap;
		uint8_t* io_bitmap;

		void* msr_bitmap_phys;
		void* io_bitmap_phys;

		uint8_t* host_simd;
		uint8_t* guest_simd;
		x86_64::paging::context* npt;

		friend struct vspace;
	}; 

	struct vspace {
		vspace();
		~vspace();

		void map(uint64_t host_phys, uint64_t guest_phys);

		private:
		std::mutex lock;
		x86_64::paging::context context;

		friend struct vcpu;
	};
} // namespace x86_64::amd::svm



#endif
