#ifndef SIGMA_GENERIC_VIRT_H
#define SIGMA_GENERIC_VIRT_H

#include <Sigma/common.h>
#include <Sigma/misc/misc.h>

#include <Sigma/types/vector.h>
#include <klibcxx/utility.hpp>

namespace virt {
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

		union {
			struct {
				uint16_t port;
				uint32_t value;
				uint8_t width;
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

	enum {
		vCtlCreateVcpu = 0,
		vCtlRunVcpu = 1,
		vCtlCreateVspace = 2,
		vCtlMapVspace = 3,
	};
	
	uint64_t vctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);


	enum class virt_types {Svm, None};
	struct vspace {
		vspace();
		~vspace();

		void map(uint64_t guest_phys, uint64_t host_phys);

		virt_types type;
		void* ptr;
	};

	struct vcpu {
		vcpu(vspace* space);
		~vcpu();

		void run(virt::vexit* vexit);

		virt_types type;
		void* ptr;
	};
}

#endif
