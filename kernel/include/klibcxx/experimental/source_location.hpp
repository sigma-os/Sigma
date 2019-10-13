#ifndef SIGMA_KLIBCXX_SOURCE_LOCATION
#define SIGMA_KLIBCXX_SOURCE_LOCATION

#include <klibcxx/common.hpp>
#include <cstdint>

namespace KLIBCXX_NAMESPACE_NAME::experimental {
    struct source_location {
        public:
        constexpr source_location() noexcept : _line(0), _column(0), _file_name("Unknown"), _function_name("Unknown") {}
        source_location(const source_location& other) = default;
        source_location(source_location&& other) = default;

        // Defined per compiler
        #ifdef __GNUC__
        static constexpr source_location current(const char* __file = __builtin_FILE(),
                                                 const char* __func = __builtin_FUNCTION(), 
                                                 uint_least32_t __line = __builtin_LINE(),
                                                 uint_least32_t __col = 0) noexcept {
            source_location loc{};
            loc._file_name = __file;
            loc._function_name = __func;
            loc._line = __line;
            loc._column = __col;
            return loc;
        }
        #else
        #error "Compiling with unknown compiler";
        #endif

        constexpr std::uint_least32_t line() const noexcept {
            return _line;
        }

        constexpr std::uint_least32_t column() const noexcept {
            return _column;
        }

        constexpr const char* file_name() const noexcept {
            return _file_name;
        }

        constexpr const char* function_name() const noexcept {
            return _function_name;
        }

        private:
        std::uint_least32_t _line, _column;
        const char* _file_name;
        const char* _function_name;
    };
}

#endif // !SIGMA_KLIBCXX_SOURCE_LOCATION
