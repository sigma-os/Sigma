#ifndef SIGMA_KERNEL_COMPILER
#define SIGMA_KERNEL_COMPILER

#if __cpp_attributes

#if __has_cpp_attribute(maybe_unused)
#define MAYBE_UNUSED_ATTRIBUTE [[maybe_unused]]
#else
#define MAYBE_UNUSED_ATTRIBUTE
#endif

#if __has_cpp_attribute(nodiscard)
#define NODISCARD_ATTRIBUTE [[nodiscard]]
#else
#define NODISCARD_ATTRIBUTE
#endif

#if __has_cpp_attribute(noreturn)
#define NORETURN_ATTRIBUTE [[noreturn]]
#else
#define NORETURN_ATTRIBUTE
#endif

#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH_ATTRIBUTE [[fallthrough]]
#else
#define FALLTHROUGH_ATTRIBUTE
#endif

// Compiler specific attributes
#ifdef __GNUC__
#define PACKED_ATTRIBUTE [[gnu::packed]]
#define NOINLINE_ATTRIBUTE [[gnu::noinline]]
#define ALWAYSINLINE_ATTRIBUTE [[gnu::always_inline]]
#else
#error "Unknown compiler"
#endif


#else
// At least try to use compiler legacy attributes, since there aren't any standardised ones 
#ifdef __GNUC__
#define MAYBE_UNUSED_ATTRIBUTE __attribute__((unused))
#define NODISCARD_ATTRIBUTE // Doesn't seem to exist
#define NORETURN_ATTRIBUTE __attribute__((noreturn))
#define FALLTHROUGH_ATTRIBUTE __attribute__((fallthrough))

#define PACKED_ATTRIBUTE __attribute__((packed))
#define NOINLINE_ATTRIBUTE __attribute__((noinline))
#define ALWAYSINLINE_ATTRIBUTE __attribute__((always_inline))
#else
#error "Unknown compiler"
#endif

#endif

#if defined(__GNUC__) || defined(__clang__) // both clang and gcc support __PRETTY_FUNCTION__
#define SIGMA_FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER) // Check for MSVC
#undef SIGMA_FUNCTION_NAME
#define SIGMA_FUNCTION_NAME __FUNCSIG__
#else // If noone supports it just use __func__ i guess
#define SIGMA_FUNCTION_NAME __func__
#endif

#endif