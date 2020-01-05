#pragma once

#include <stdint.h>
#include <libsigma/device.h>

#include "nvme/queue.hpp"
#include "nvme/regs.hpp"

namespace nvme {
    class controller {
        public:
        controller(libsigma_resource_region_t region);

        enum class shutdown_types {
            NormalShutdown,
            AbruptShutdown,
        };

        void set_power_state(shutdown_types type);
        void reset_subsystem();

        private:
        bool identify(regs::identify_info* info);

        bool weighted_round_robin_supported;
        bool nssr_supported;

        size_t doorbell_stride;
        size_t n_queue_entries;

        volatile regs::bar* base;

        queue_pair admin_queue;
    };
}