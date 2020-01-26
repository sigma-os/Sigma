#pragma once

#include <stdint.h>
#include <libsigma/device.h>
#include <vector>
#include <unordered_map>

#include <nvme/common.hpp>
#include <nvme/queue.hpp>
#include <nvme/regs.hpp>

namespace nvme {
    class io_controller {
        public:
        io_controller(libsigma_resource_region_t region);

        enum class shutdown_types {
            NormalShutdown,
            AbruptShutdown,
        };

        void set_power_state(shutdown_types type);
        void reset_subsystem();
        
        std::vector<uint8_t> read_sector(nsid_t nsid, uint64_t lba);

        private:
        bool set_features(uint8_t fid, uint32_t data);
        bool register_queue_pair(queue_pair& pair);
        bool identify_controller(regs::controller_identify_info* info);
        bool identify_namespace(nsid_t nsid, regs::namespace_identify_info* info);

        void print_identify_info(regs::controller_identify_info& info);

        std::vector<nsid_t> get_active_namespaces(nsid_t max_nsid);

        bool weighted_round_robin_supported;

        size_t doorbell_stride;
        size_t n_queue_entries;

        volatile regs::bar* base;

        queue_pair admin_queue;
        queue_pair io_queue;

        std::string path;

        struct ns {
            uint64_t n_lbas;
            uint64_t sector_size;
            std::string path;
        };

        std::unordered_map<nsid_t, ns> namespaces;
    };
}