#ifndef LIBKBUS_H
#define LIBKBUS_H

#if !defined(__cplusplus)
#error "Not compiling as C++"
#endif

#include <stdint.h>
#include <string>
#include <vector>

namespace kbus
{
    using object_id = uint64_t;
    object_id allocate_object();

    class object {
        public:
        object();
        object(object_id id);

        std::string get_attribute(std::string key);
        void set_attribute(std::string key, std::string value);

        private:
        object_id id;
    };

    std::vector<object> find_devices(std::string query);
} // namespace kbus

#endif