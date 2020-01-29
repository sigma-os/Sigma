#pragma once

#include <stdint.h>
#include <stddef.h>
#include <libsigma/sys.h>
#include <libdriver/bit.hpp>

#include <nvme/common.hpp>
#include <nvme/regs.hpp>

namespace nvme
{       
    class queue_pair {
        public:
        queue_pair() = default;
        queue_pair(size_t n_entries, uint16_t* submission_doorbell, uint16_t* completion_doorbell, qid_t qid);

        cid_t send_command(regs::command* cmd);
        bool send_and_wait(regs::command* cmd);

        size_t get_n_entries();
        uintptr_t get_submission_phys_base();
        uintptr_t get_completion_phys_base();
        qid_t get_qid(){
            return qid;
        }

        private:
        size_t n_entries;
        qid_t qid;

        static constexpr uint64_t n_commands = pow(2, 16); 
        bitmap<n_commands> available_cids;

        struct submission_info {
            submission_info() = default;
            submission_info(size_t n_entries, uint16_t* doorbell);
            volatile regs::command* queue;
            volatile uint16_t* doorbell;

            uint16_t head, tail;
            libsigma_phys_region_t region;
        };
        submission_info submission;

        struct completion_info {
            completion_info() = default;
            completion_info(size_t n_entries, uint16_t* doorbell);
            volatile regs::completion* queue;
            volatile uint16_t* doorbell;

            uint16_t head, tail;
            uint8_t expected_phase;
            libsigma_phys_region_t region;
        };
        completion_info completion;
    };
} // namespace nvme
