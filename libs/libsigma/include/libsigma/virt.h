#ifndef LIBSIGMA_VIRT_H
#define LIBSIGMA_VIRT_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#include <stdbool.h>
#elif defined(__STDC__)
#include <stdint.h>
#include <stdbool.h>
#else 
#error "Compiling libsigma/virt.h on unknown language"
#endif

enum {
	vCtlExitReasonHlt = 0,
	vCtlExitReasonRdtsc,
	vCtlExitReasonPortRead,
	vCtlExitReasonPortWrite,
	vCtlExitReasonMsrRead,
	vCtlExitReasonMsrWrite,
	vCtlExitReasonSRegRead,
	vCtlExitReasonSRegWrite,
	vCtlExitReasonNestedPageFault,
	vCtlExitReasonHypercall,
	vCtlExitReasonInterrupt,

	vCtlExitReasonInvalidInternalState = -1
};

enum {
	vCtlExitRegNumberCr0,
	vCtlExitRegNumberCr1,
	vCtlExitRegNumberCr2,
	vCtlExitRegNumberCr3,
	vCtlExitRegNumberCr4,
	vCtlExitRegNumberCr5,
	vCtlExitRegNumberCr6,
	vCtlExitRegNumberCr7,
	vCtlExitRegNumberCr8,
	vCtlExitRegNumberCr9,
	vCtlExitRegNumberCr10,
	vCtlExitRegNumberCr11,
	vCtlExitRegNumberCr12,
    vCtlExitRegNumberCr13,
	vCtlExitRegNumberCr14,
	vCtlExitRegNumberCr15,
	vCtlExitRegNumberGdtr,
	vCtlExitRegNumberIdtr,
	vCtlExitRegNumberLdtr,
	vCtlExitRegNumberTr,
};

struct vexit {
	uint64_t reason;
	uint8_t opcode[15];
	uint8_t opcode_length;

	uint8_t interrupt_number;

	union {
		struct {
			uint16_t port;
			uint32_t value;
			bool repeated;
			bool string;
		} port;
		struct {
			uint32_t number;
			uint64_t value;
		} msr;
		struct {
			uint8_t reg_number;
			uint64_t value;
		} sreg;
		struct {
			uint64_t phys_addr;
			uint64_t value;
			uint8_t len;
			bool write;
		} npf;
	};
};

struct vselector {
	uint16_t selector;
	uint16_t attrib;
	uint32_t limit;
	uint64_t base;
};

struct vdtable {
	uint64_t base;
	uint16_t limit;
};

struct vregs {
	uint64_t rax, rbx, rcx, rdx, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
	uint64_t rsp, rbp, rip, rflags;

	uint64_t cr0, cr2, cr3, cr4, cr8;
	uint64_t efer;

	struct vselector cs, ds, ss, es, fs, gs;
	struct vselector ldtr, tr;
	struct vdtable gdtr, idtr;
};

enum {
	vCtlCreateVcpu = 0,
	vCtlRunVcpu,
	vCtlGetRegs,
	vCtlSetRegs,
	vCtlCreateVspace,
    vCtlMapVspace,
	vCtlMapVspacePhys,
};

uint64_t vctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

#if defined(__cplusplus)
}
#endif

#endif