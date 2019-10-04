#include <Sigma/debug.h>

struct frame {
    frame* rbp;
    uint64_t* rip;
};

void debug::trace_stack(MAYBE_UNUSED_ATTRIBUTE uint8_t levels){
    #if defined(DEBUG)
    frame* current = static_cast<frame*>(__builtin_frame_address(0));

	debug_printf("Starting Stack Trace\n");
	for(size_t i = 0; i < levels; i++) {
		debug_printf("  Level %d: RIP [%x]\n", i, current->rip);
		if(!common::is_canonical((uint64_t)current->rbp) || current->rbp == nullptr)
			break;
		current = current->rbp;
	}

	debug_printf("End of Stack Trace\n");
#endif
}
