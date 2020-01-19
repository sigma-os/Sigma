#ifndef SIGMA_GENERIC_VIRT_H
#define SIGMA_GENERIC_VIRT_H

#include <Sigma/common.h>
#include <Sigma/misc/misc.h>

#include <Sigma/types/vector.h>
#include <klibcxx/utility.hpp>

namespace virt {
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

		void run();

		virt_types type;
		void* ptr;
	};
	
	enum {
		vCtlCreateVcpu = 0,
		vCtlRunVcpu = 1,
		vCtlCreateVspace = 2,
		vCrlMapVspace = 3,
	};
	
	uint64_t vctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
}

#endif
