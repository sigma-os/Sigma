#include <Sigma/misc/debug.h>
#include <Sigma/proc/elf.h>

struct frame {
    frame* rbp;
    uint64_t rip;
};

void debug::trace_stack(uintptr_t rbp){
    #if defined(DEBUG)
    auto* current = (frame*)rbp;

	debug_printf("Starting Stack Trace\n");
	for(size_t i = 0; i < UINT64_MAX; i++) {
		if(!current)
			break;
		if(!misc::is_canonical((uint64_t)current->rbp) || current->rbp == nullptr || !misc::is_canonical(current->rip) || current->rip == 0)
			break;
		
		const auto [name, addr] = proc::elf::get_symbol(current->rip);
		if(addr != 0)
			debug_printf("  %d: RIP [%x] -> %s+%x\n", i, current->rip, name, current->rip - addr);
		else
			debug_printf("  %d: RIP [%x]\n", i, current->rip);
		
		current = current->rbp;
	}

	debug_printf("End of Stack Trace\n");
	#endif
}

void debug::trace_stack(){
	trace_stack((uintptr_t)__builtin_frame_address(0));
}

// TODO: Instead of a set x value use another compile time source
uintptr_t __stack_chk_guard = misc::compile_time_prng(43243, 0xC6AF8B7718FFF628, 8243423);
 
NORETURN_ATTRIBUTE void __stack_chk_fail()
{
	PANIC("Stack smashing detected");

	while(1)
		;
}

#if defined(SIGMA_UBSAN)
using namespace __ubsan;

C_LINKAGE void __ubsan_handle_add_overflow(overflow_data* data, uintptr_t lhs, uintptr_t rhs) {
	debug_printf("[UBSan]: Add overflow\n");
	data->print("	");
	debug_printf("	lhs: %x, rhs: %x\n", lhs, rhs);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}

C_LINKAGE void __ubsan_handle_sub_overflow(overflow_data* data, uintptr_t lhs, uintptr_t rhs) {
	debug_printf("[UBSan]: Sub overflow\n");
	data->print("	");
	debug_printf("	lhs: %x, rhs: %x\n", lhs, rhs);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}

C_LINKAGE void __ubsan_handle_pointer_overflow(overflow_data* data, uintptr_t lhs, uintptr_t rhs) {
	debug_printf("[UBSan]: Pointer overflow\n");
	data->print("	");
	debug_printf("	lhs: %x, rhs: %x\n", lhs, rhs);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_mul_overflow(overflow_data* data, uintptr_t lhs, uintptr_t rhs) {
	debug_printf("[UBSan]: Mul overflow\n");
	data->print("	");
	debug_printf("	lhs: %x, rhs: %x\n", lhs, rhs);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_divrem_overflow(overflow_data* data, uintptr_t lhs, uintptr_t rhs) {
	debug_printf("[UBSan]: Divide Remainder overflow\n");
	data->print("	");
	debug_printf("	lhs: %x, rhs: %x\n", lhs, rhs);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_negate_overflow(overflow_data* data, uintptr_t old) {
	debug_printf("[UBSan]: Negate overflow\n");
	data->print("	");
	debug_printf("	lhs: %x\n", old);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_shift_out_of_bounds(shift_out_of_bounds_data* data, uintptr_t lhs, uintptr_t rhs) {
	debug_printf("[UBSan]: Shift out of bounds overflow\n");
	data->print("	");
	debug_printf("	 lhs: %x, rhs: %x\n", lhs, rhs);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_out_of_bounds(out_of_bounds_data* data, uintptr_t index) {
	debug_printf("[UBSan]: Shift out of bounds overflow\n");
	data->print("	");
	debug_printf("	index: %x\n", index);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_nonnull_return(non_null_return_data* data, source_location* loc) {
	debug_printf("[UBSan]: Nonnull return\n");
	data->print("	");
	debug_printf("	Return loc: ");
	loc->print("	");
	debug_printf("\n");
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}

C_LINKAGE void __ubsan_handle_type_mismatch_v1(type_mismatch_data_v1* data, uintptr_t ptr) {
	debug_printf("[UBSan]: Type Mismatch\n");
	if(!ptr) {
        debug_printf("	Null pointer access\n");
    } else if (ptr &  ((1 << data->log_alignment) - 1)) {
    	debug_printf("	Unaligned memory access\n");
		debug_printf("	ptr: %x\n", ptr);
    } else {
    	debug_printf("	Insufficient size\n");
		debug_printf("	%s address %x with insufficient space for object of type ↓\n", type_check_kind[data->type_check_kind], ptr);
		data->type->print("	");
	}
	data->loc.print("	 ");
	debug_printf("	aligned at: %x\n", misc::pow(2, data->log_alignment));
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_vla_bound_not_positive(vla_bound_data* data, uintptr_t bound) {
	debug_printf("[UBSan]: vla bound not positive\n");
	data->print("	");
	debug_printf("	bound: %x\n", bound);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_load_invalid_value(invalid_value_data* data, uintptr_t val) {
	debug_printf("[UBSan]: load invalid value\n");
	data->print("	");
	debug_printf("	value: %x\n", val);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_builtin_unreachable(unreachable_data* data) {
	debug_printf("[UBSan]: builtin unreachable\n");
	data->print("	");
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_nonnull_arg(nonnull_arg_data* data) {
	debug_printf("[UBSan]: nonnull arg\n");
	data->print("	");
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}
 
C_LINKAGE void __ubsan_handle_type_mismatch(type_mismatch_data* data, uintptr_t ptr) {
	debug_printf("[UBSan]: Type Mismatch\n");
	if(!ptr) {
        debug_printf("	Null pointer access\n");
    } else if (ptr & (data->alignment - 1)) {
    	debug_printf("	Unaligned memory access\n");
		debug_printf("	ptr: %x\n", ptr);
    } else {
    	debug_printf("	Insufficient size\n");
		debug_printf("	%s address %x with insufficient space for object of type ↓\n", type_check_kind[data->type_check_kind], ptr);
		data->type->print("	");
	}
	data->loc.print("	");
	debug_printf("aligned at: %x\n", data->alignment);
	#if defined(PANIC_ON_UBSAN_ERROR)
	PANIC("UBSan PANIC");
	#endif
}

#endif