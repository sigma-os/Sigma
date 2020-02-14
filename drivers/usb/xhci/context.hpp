#pragma once

#include <libsigma/sys.h>
#include <sys/mman.h>
#include <stdio.h>

#include "trb.hpp"

namespace xhci
{
    struct [[gnu::packed]] slot_context {
        uint32_t route_string : 20;
        uint32_t speed : 4;
        uint32_t reserved : 1;
        uint32_t multi_tt : 1;
        uint32_t hub : 1;
        uint32_t context_entries : 5;

        uint32_t max_exit_latency : 16;
        uint32_t root_hub_port_number : 8;
        uint32_t number_of_ports : 8;

        uint32_t parent_hub_slot_id : 8;
        uint32_t parent_port_number : 8;
        uint32_t tt_think_time : 2;
        uint32_t reserved_0 : 4;
        uint32_t interrupter_target : 10;

        uint32_t usb_device_address : 8;
        uint32_t reserved_1 : 19;
        uint32_t slot_state : 5; 

        uint32_t reserved_2[4];
    };

    namespace ep_types
    {
        enum {
            epTypeInvalid = 0,
            epTypeIsochOut,
            epTypeBulkOut,
            epTypeInterruptOut,
            epTypeControlBi,
            epTypeIsochIn,
            epTypeBulkIn,
            epTypeInterruptIn,
        };
    } // namespace ep_types
    

    struct [[gnu::packed]] ep_context {
        uint32_t ep_state : 3;
        uint32_t reserved : 5;
        uint32_t mult : 2;
        uint32_t max_primary_streams : 5;
        uint32_t linear_stream_array : 1;
        uint32_t interval : 8;
        uint32_t max_esit_payload_hi : 8;

        uint32_t reserved_0 : 1;
        uint32_t error_count : 2;
        uint32_t endpoint_type : 3;
        uint32_t reserved_1 : 1;
        uint32_t host_initiate_disable : 1;
        uint32_t max_burst_size : 8;
        uint32_t max_packet_size : 16;
        
        uint64_t tr_dequeue_ptr;

        uint32_t average_trb_len : 16;
        uint32_t max_esit_payload_low : 16;

        uint32_t reserved_3[4];
    };

    struct [[gnu::packed]] input_control_ctx {
        uint32_t drop_flags;
        uint32_t add_flags;
        uint32_t reserved[6];
    };

    enum {
        contextTypeControl,
        contextTypeDevice = 1,
        contextTypeInput,
    };

    struct in_context {
        /* Format of In Context
            - Input Control Context
            - Slot Context
            - EP0 Context
            - EP1 Out Context
            - EP1 In Context
            ...
            - EP15 Out Context
            - EP15 Out Context
        */
        in_context() = default;
        in_context(size_t context_size, size_t size): context_size{context_size}, size{size} {
            libsigma_phys_region_t region = {};
            if(libsigma_get_phys_region(size, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
                printf("xhci: Failed to allocate physical region for in context\n");
                return;
            }

            this->addr = (uint8_t*)region.virtual_addr;
            this->phys = region.physical_addr;
        }

        input_control_ctx* get_control_ctx(){
            return (input_control_ctx*)addr;
        }

        slot_context* get_slot_ctx(){
            return (slot_context*)((uintptr_t)addr + context_size);
        }

        ep_context* get_ep0_ctx(){
            return (ep_context*)((uintptr_t)addr + (context_size * 2));
        }

        ep_context* get_ep_ctx(int n){
            return (ep_context*)((uintptr_t)addr + (context_size * (n + 1)));
        }
        
        size_t context_size, size;
        uint8_t* addr;
        uintptr_t phys;
    };

    struct out_context {
        /* Format of Out Context
            - Slot Context
            - EP0 Context
            - EP1 Out Context
            - EP1 In Context
            ...
            - EP15 Out Context
            - EP15 Out Context
        */
       out_context() = default;
        out_context(size_t context_size, size_t size): context_size{context_size}, size{size} {
            libsigma_phys_region_t region = {};
            if(libsigma_get_phys_region(size, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
                printf("xhci: Failed to allocate physical region for out context\n");
                return;
            }

            this->addr = (uint8_t*)region.virtual_addr;
            this->phys = region.physical_addr;
        }

        slot_context* get_slot_ctx(){
            return (slot_context*)addr;
        }

        ep_context* get_ep0_ctx(){
            return (ep_context*)((uintptr_t)addr + (context_size * 1));
        }

        ep_context* get_ep_ctx(int n){
            return (ep_context*)((uintptr_t)addr + (context_size * n));
        }
        
        size_t context_size, size;
        uint8_t* addr;
        uintptr_t phys;
    };
} // namespace xhci
