#ifndef SIGMA_DEBUG
#define SIGMA_DEBUG

#include <Sigma/common.h>
#include <klibc/stdio.h>

namespace debug
{
    void trace_stack(uint8_t levels);
} // namespace debug

#if defined(SIGMA_UBSAN)

namespace __ubsan {
	enum class type_kinds : uint16_t {TK_INTEGER = 0x0, TK_FLOAT = 0x1, TK_UNKNOWN = 0xFFFF};

	struct type_descriptor {
		type_kinds kind;
		uint16_t type_info;
		char type_name[];
		void print(const char* prefix){
			debug_printf("%s Type Descriptor: name: %s, info: %x, ", prefix, type_name, type_info);
			switch (kind)
			{
			case type_kinds::TK_INTEGER:
				debug_printf("Type: Integer");
				break;

			case type_kinds::TK_FLOAT:
				debug_printf("Type: Float");
				break;

			case type_kinds::TK_UNKNOWN:
				debug_printf("Type: Unknown");
				break;
			
			default:
				debug_printf("Type: Undefined [%x]", misc::as_integer(kind));
				break;
			}
			debug_printf("\n");
		}
	};

	struct source_location {
		const char* filename;
		uint32_t line;
		uint32_t column;
		void print(const char* prefix){
			debug_printf("%s Location Descriptor: file: %s, line: %d, column: %d\n", prefix, filename, line, column);
		}
	};

	struct overflow_data {
		source_location loc;
		type_descriptor* type;
		void print(const char* prefix){
			this->type->print(prefix);
			this->loc.print(prefix);
		}
	};

	struct shift_out_of_bounds_data {
		source_location loc;
		type_descriptor* lhs;
		type_descriptor* rhs;
		void print(const char* prefix){
			debug_printf("%s rhs: ", prefix);
			this->rhs->print("");
			debug_printf("%s lhs: ", prefix);
			this->lhs->print("");
			this->loc.print(prefix);
		}
	};

	struct out_of_bounds_data {
		source_location loc;
		type_descriptor* array;
		type_descriptor* index;
		void print(const char* prefix){
			debug_printf("%s Array: ", prefix);
			this->array->print("");
			debug_printf("%s Index: ", prefix);
			this->array->print("");
			this->loc.print(prefix);
		}
	};

	struct non_null_return_data {
		source_location attribute_loc;
		void print(const char* prefix){
			this->attribute_loc.print(prefix);
		}
	};

	struct type_mismatch_data_v1 {
		source_location loc;
		type_descriptor* type;
		uint8_t log_alignment;
		uint8_t type_check_kind;
		void print(const char* prefix){
			this->loc.print(prefix);
			this->type->print(prefix);
			debug_printf("%s, alignment: %d, type_check_kind: %x\n", prefix, misc::pow(2, log_alignment), this->type_check_kind);
		}
	};

	struct type_mismatch_data {
		source_location loc;
		type_descriptor* type;
		uint64_t alignment;
		uint8_t type_check_kind;
	};

	struct vla_bound_data {
		source_location loc;
		type_descriptor* type;
		void print(const char* prefix){
			this->loc.print(prefix);
			this->type->print(prefix);
		}
	};

	struct invalid_value_data {
		source_location loc;
		type_descriptor* type;
		void print(const char* prefix){
			this->loc.print(prefix);
			this->type->print(prefix);
		}
	};

	struct unreachable_data {
		source_location loc;
		void print(const char* prefix){
			this->loc.print(prefix);
		}
	};

	struct nonnull_arg_data {
		source_location loc;
		void print(const char* prefix){
			this->loc.print(prefix);
		}
	};

	enum class type_names : int { add_overflow = 0,
    							  sub_overflow,
    							  mul_overflow,
   								  divrem_overflow,
    							  negate_overflow,
    							  shift_out_of_bounds,
    							  out_of_bounds,
    							  nonnull_return,
    							  type_mismatch_v1,
    							  vla_bound_not_positive,
    							  load_invalid_value,
    							  builtin_unreachable,
    							  nonnull_arg,
    							  pointer_overflow,
    							  type_mismatch };

	/*static const char* type_strs[] = {
    	"add_overflow",
    	"sub_overflow",
    	"mul_overflow",
    	"divrem_overflow",
    	"negate_overflow",
    	"shift_out_of_bounds",
    	"out_of_bounds",
    	"nonnull_return",
    	"type_mismatch_v1",
    	"vla_bound_not_positive",
    	"load_invalid_value",
    	"builtin_unreachable",
    	"nonnull_arg",
    	"pointer_overflow",
    	"type_mismatch"
	};*/

	constexpr const char* type_check_kind[] = {
    	"load of",
    	"store to",
    	"reference binding to",
    	"member access within",
    	"member call on",
    	"constructor call on",
    	"downcast of",
    	"downcast of",
    	"upcast of",
    	"cast to virtual base of",
	};

	//#define PANIC_ON_UBSAN_ERROR
}

C_LINKAGE void __ubsan_handle_add_overflow(__ubsan::overflow_data* data, uintptr_t lhs, uintptr_t rhs);
C_LINKAGE void __ubsan_handle_sub_overflow(__ubsan::overflow_data* data, uintptr_t lhs, uintptr_t rhs);
C_LINKAGE void __ubsan_handle_pointer_overflow(__ubsan::overflow_data* data, uintptr_t lhs, uintptr_t rhs);
C_LINKAGE void __ubsan_handle_mul_overflow(__ubsan::overflow_data* data, uintptr_t lhs, uintptr_t rhs);    
C_LINKAGE void __ubsan_handle_divrem_overflow(__ubsan::overflow_data* data, uintptr_t lhs, uintptr_t rhs);
C_LINKAGE void __ubsan_handle_negate_overflow(__ubsan::overflow_data* data, uintptr_t old);    
C_LINKAGE void __ubsan_handle_shift_out_of_bounds(__ubsan::shift_out_of_bounds_data* data, uintptr_t lhs, uintptr_t rhs);
C_LINKAGE void __ubsan_handle_out_of_bounds(__ubsan::out_of_bounds_data* data, uintptr_t index);
C_LINKAGE void __ubsan_handle_nonnull_return(__ubsan::non_null_return_data* data, __ubsan::source_location* loc);
C_LINKAGE void __ubsan_handle_type_mismatch_v1(__ubsan::type_mismatch_data_v1* data, uintptr_t ptr);
C_LINKAGE void __ubsan_handle_vla_bound_not_positive(__ubsan::vla_bound_data* data, uintptr_t bound);
C_LINKAGE void __ubsan_handle_load_invalid_value(__ubsan::invalid_value_data* data, uintptr_t val);
C_LINKAGE void __ubsan_handle_builtin_unreachable(__ubsan::unreachable_data* data);
C_LINKAGE void __ubsan_handle_nonnull_arg(__ubsan::nonnull_arg_data* data);
C_LINKAGE void __ubsan_handle_type_mismatch(__ubsan::type_mismatch_data* data, uintptr_t ptr);

#endif

#endif