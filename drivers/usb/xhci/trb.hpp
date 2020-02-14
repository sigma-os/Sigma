#pragma once

#include <stdint.h>
#include <stddef.h>
#include <libsigma/sys.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <utility>
#include <unordered_map>
#include <atomic>

namespace xhci
{
    namespace trb_types
    {
        enum {
            reserved = 0,
            normal,
            setupStage,
            dataStage,
            statusStage,
            isochronous,
            link,
            eventData,
            noOp,
            enableSlotCommand,
            disableSlotCommand,
            addressDeviceCommand,
            configureEndpointCommand,
            evaluateContextCommand,
            resetEndpointCommand,
            stopEndpointCommand,
            setTrDequeueCommand,
            resetDeviceCommand,
            forceEventCommand,
            negotiateBandwidthCommand,
            setLatencyToleranceValueCommand,
            getPortBandwidthCommand,
            forceHeaderCommand,
            noOpCommand,
            getExtendedPropertyCommand,
            setExtendedPropertyCommand,

            transferEvent = 32,
            commandCompletionEvent,
            portStatusChangeEvent,
            bandwidthRequestEvent,
            doorbellEvent,
            hostControllerEvent,
            deviceNotificationEvent,
            MFINDEXWrapEvent,
        };
    } // namespace trb_types

    namespace trb_completion_codes
    {
        enum {
            invalid = 0,
            success,
            dataBufferError,
            babbleDetectedError,
            usbTransactionError,
            trbError,
            stallError,
            resourceError,
            bandwidthError,
            noSlotsAvailableError,
        };
    } // namespace name
    
    

    struct [[gnu::packed]] setup_stage_trb {
        setup_stage_trb(): trb_type{trb_types::setupStage} {}
        uint8_t bmRequestType;
        uint8_t bRequest;
        uint16_t wValue;

        uint16_t wIndex;
        uint16_t wLength;

        uint16_t trb_transfer_len;

        uint16_t reserved : 7;
        uint16_t interrupter : 9;

        struct {
            uint32_t cycle : 1;
            uint32_t reserved_0 : 4;
            uint32_t irq_on_completion : 1;
            uint32_t immediate_data : 1;
            uint32_t reserved_1 : 3;
            uint32_t trb_type : 6;
            uint32_t transfer_type : 2;
            uint32_t reserved_2 : 14;
        };
    };
    static_assert(sizeof(setup_stage_trb) == 16);

    struct [[gnu::packed]] data_stage_trb {
        data_stage_trb(): trb_type{trb_types::dataStage} {}
        uint64_t data_ptr;

        uint16_t transfer_len;
        uint16_t td_size : 7;
        uint16_t interrupter : 9;

        struct {
            uint32_t cycle : 1;
            uint32_t evaluate_next_trb : 1;
            uint32_t irq_on_short_packet : 1;
            uint32_t no_snoop : 1;
            uint32_t chain : 1;
            uint32_t irq_on_completion : 1;
            uint32_t immediate_data : 1;
            uint32_t reserved : 3;
            uint32_t trb_type : 6;
            uint32_t direction : 1;
            uint32_t reserved_0 : 15;
        };
    };
    static_assert(sizeof(data_stage_trb) == 16);

    struct [[gnu::packed]] status_stage_trb {
        status_stage_trb(): trb_type{trb_types::statusStage} {}
        uint32_t reserved[2];

        uint32_t reserved_0 : 22;
        uint32_t interrupter : 10;

        uint32_t cycle : 1;
        uint32_t evaluate_next_trb : 1;
        uint32_t reserved_1 : 2;
        uint32_t chain : 1;
        uint32_t irq_on_completion : 1;
        uint32_t reserved_2 : 4;
        uint32_t trb_type : 6;
        uint32_t direction : 1;
        uint32_t reserved_3 : 15;
    };
    static_assert(sizeof(status_stage_trb) == 16);

    struct [[gnu::packed]] command_completion_event_trb {
        uint64_t trb_ptr;
        struct {
            uint32_t parameter : 24;
            uint32_t code : 8;
        };
        struct {
            uint32_t cycle : 1;
            uint32_t reserved : 9;
            uint32_t trb_type : 6;
            uint32_t vfid : 8;
            uint32_t slot_id : 8;
        };
    };
    static_assert(sizeof(command_completion_event_trb) == 16);

    struct [[gnu::packed]] transfer_completion_event_trb {
        uint64_t trb_ptr;
        struct {
            uint32_t trb_transfer_length : 24;
            uint32_t code : 8;
        };
        struct {
            uint32_t cycle : 1;
            uint32_t reserved : 1;
            uint32_t event_data : 1;
            uint32_t reserved_0 : 7;
            uint32_t trb_type : 6;
            uint32_t endpoint_id : 5;
            uint32_t reserved_1 : 3;
            uint32_t slot_id : 8;
        };
    };
    static_assert(sizeof(transfer_completion_event_trb) == 16);

    struct [[gnu::packed]] noop_command_trb {
        noop_command_trb(): trb_type{trb_types::noOpCommand} {}
        uint32_t reserved_1[3];
        struct {
            uint32_t cycle : 1;
            uint32_t reserved : 9;
            uint32_t trb_type : 6;
            uint32_t reserved_0 : 16;
        };
    };
    static_assert(sizeof(noop_command_trb) == 16);

    struct [[gnu::packed]] enable_slot_command_trb {
        enable_slot_command_trb(): trb_type{trb_types::enableSlotCommand} {}
        uint32_t reserved[3];
        struct {
            uint32_t cycle : 1;
            uint32_t reserved_0 : 9;
            uint32_t trb_type : 6;
            uint32_t slot_type : 5;
            uint32_t reserved_1 : 11;
        };
    };
    static_assert(sizeof(enable_slot_command_trb) == 16);

    struct [[gnu::packed]] disable_slot_command_trb {
        disable_slot_command_trb(): trb_type{trb_types::disableSlotCommand} {}
        uint32_t reserved_1[3];
        struct {
            uint32_t cycle : 1;
            uint32_t reserved : 9;
            uint32_t trb_type : 6;
            uint32_t reserved_0 : 8;
            uint32_t slot_id : 8;
        };
    };
    static_assert(sizeof(disable_slot_command_trb) == 16);

    struct [[gnu::packed]] address_device_command_trb {
        address_device_command_trb(): trb_type{trb_types::addressDeviceCommand} {}
        uint64_t in_ctx_ptr;
        uint32_t reserved;
        struct {
            uint32_t cycle : 1;
            uint32_t reserved_0 : 8;
            uint32_t block_set_address_req : 1;
            uint32_t trb_type : 6;
            uint32_t reserved_1 : 8;
            uint32_t slot_id : 8;
        };
    };
    static_assert(sizeof(address_device_command_trb) == 16);

    struct [[gnu::packed]] configure_ep_trb {
        configure_ep_trb(): trb_type{trb_types::configureEndpointCommand} {}
        uint64_t in_ctx_ptr;
        uint32_t reserved;
        struct {
            uint32_t cycle : 1;
            uint32_t reserved_0 : 8;
            uint32_t deconfigure : 1;
            uint32_t trb_type : 6;
            uint32_t reserved_1 : 8;
            uint32_t slot_id : 8;
        };
    };
    static_assert(sizeof(configure_ep_trb) == 16);

    struct link_trb {
        uint64_t ring_segment_ptr;
        struct {
            uint32_t reserved : 22;
            uint32_t interrupter_target : 10;
        };
        struct {
            uint32_t cycle : 1;
            uint32_t toggle_cycle : 1;
            uint32_t reserved_0 : 2;
            uint32_t chain : 1;
            uint32_t irq_on_completion : 1;
            uint32_t reserved_1 : 4;
            uint32_t trb_type : 6;
            uint32_t reserved_2 : 16;
        };
    };
    static_assert(sizeof(link_trb) == 16);

    union [[gnu::packed]] raw_trb {
        struct {
            uint32_t data[4];
        };
        struct {
            uint32_t reserved_1[3];
            struct {
                uint32_t cycle : 1;
                uint32_t reserved : 9;
                uint32_t trb_type : 6;
                uint32_t reserved_0 : 16;
            };
        };
    };
    static_assert(sizeof(raw_trb) == 16);

    enum {
        ringTypeCommand,
        ringTypeTransfer,
        ringTypeEvent,
    };

    class command_ring;
    class event_ring;
    class transfer_ring;

    // TODO: Support multiple ring segments
    class tlb_ring {
        public:
        tlb_ring(size_t n_entries, uint64_t type): n_entries{n_entries}, type{type} {
            if(libsigma_get_phys_region(this->n_entries * sizeof(raw_trb), PROT_READ | PROT_WRITE, MAP_ANON, &this->region)){
                printf("xhci: Failed to allocate physical region for trb ring\n");
                return;
            }

            this->entries = (raw_trb*)this->region.virtual_addr;

            this->enqueue = (uint64_t)this->entries;
            this->dequeue = (uint64_t)this->entries;

            this->cycle = 1;

            if(type != ringTypeEvent){
                auto* link = (link_trb*)(this->entries + this->n_entries - 1);
                *link = {};

                link->ring_segment_ptr = region.physical_addr; // Circular ring
                link->cycle = 1;
                link->trb_type = trb_types::link;
            }
        }

        ~tlb_ring() {
            // TODO
        }

        std::pair<raw_trb*, uint64_t> get_trb(){
            uint64_t enq = this->enqueue;
            uint64_t index = (enq - (uint64_t)this->entries) / 16 + 1;

            if(index == (this->n_entries - 1)){ // Wrap around
                this->enqueue = (uint64_t)this->entries;
                this->cycle ^= this->cycle;

                auto* link = (link_trb*)(this->entries + this->n_entries - 1);
                *link = {};

                link->ring_segment_ptr = region.physical_addr; // Circular ring
                link->cycle = 1;
                link->trb_type = trb_types::link;
            } else {
                this->enqueue += 16;
            }

            return {(raw_trb*)enq, index - 1};
        }

        uintptr_t get_phys_base(){
            return region.physical_addr;
        }

        size_t get_n_entries(){
            return n_entries;
        }

        private:
        uint64_t type;
        size_t n_entries;
        raw_trb* entries;
        libsigma_phys_region_t region;

        uint64_t enqueue, dequeue;

        size_t cycle;

        friend class command_ring;
        friend class event_ring;
        friend class transfer_ring;
    };

    class command_ring {
        public:
        command_ring(): ring{0x1000, ringTypeCommand} {}

        uint64_t enqueue_trb(raw_trb* trb, bool first) {
            //uint64_t index = (ring.enqueue - (uint64_t)ring.entries) / 0x10;
            auto [entry, index] = ring.get_trb();
            memcpy((void*)entry, (void*)trb, 16);

            //printf("xhci: Enqueuing command at index %ld, addr: %p\n", index, entry);

            if(!first){
                uint32_t state = (entry->data[3] & 0x1) ? 0 : 1;
                entry->data[3] |= state;
            }

            return index;
        }

        uint64_t get_phys_base(){
            return ring.get_phys_base();
        }

        size_t get_n_entries(){
            return ring.get_n_entries();
        }

        uint64_t& cycle(){
            return ring.cycle;
        }

        struct trb_event {
            command_completion_event_trb trb;
            std::atomic<bool> finished;
        };

        std::unordered_map<uint64_t, trb_event> events;
        private:
        tlb_ring ring;

        friend class tlb_ring;
    };

    class event_ring {
        public:
        event_ring(): ring{0x1000, ringTypeEvent} {}

        uint64_t get_phys_base(){
            return ring.get_phys_base();
        }
        
        size_t get_n_entries(){
            return ring.get_n_entries();
        }

        uint64_t& dequeue(){
            return ring.dequeue;
        }

        raw_trb* get_entries(){
            return ring.entries;
        }

        uint64_t& cycle(){
            return ring.cycle;
        }

        private:
        tlb_ring ring; 

        friend class tlb_ring;
    };

    class transfer_ring {
        public:
        transfer_ring(): ring{0x1000, ringTypeTransfer} {}

        uint64_t get_phys_base(){
            return ring.get_phys_base();
        }
        
        size_t get_n_entries(){
            return ring.get_n_entries();
        }

        uint64_t& enqueue(){
            return ring.enqueue;
        }

        raw_trb* get_entries(){
            return ring.entries;
        }

        uint64_t& cycle(){
            return ring.cycle;
        }

        struct trb_event {
            transfer_completion_event_trb trb;
            std::atomic<bool> finished;
        };

        std::unordered_map<uint64_t, trb_event> events;

        tlb_ring ring;
        private:
        
    };
} // namespace trb
