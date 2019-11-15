#include <Sigma/arch/x86_64/drivers/hpet.h>

#include <lai/core.h>
#include <lai/helpers/resource.h>

static x86_64::hpet::table* acpi_table = nullptr;
static uint64_t base = 0;
static uint64_t main_counter_clk = 0;
static uint64_t n_counters = 0;
static bool supports_64bit_counter = false;

static uint64_t hpet_read(uint64_t reg){
    volatile uint64_t* ptr = reinterpret_cast<volatile uint64_t*>(base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + reg);
    return *ptr;
}

static void hpet_write(uint64_t reg, uint64_t value){
    volatile uint64_t* ptr = reinterpret_cast<volatile uint64_t*>(base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + reg);
    *ptr = value;
}

void x86_64::hpet::init_hpet(){
    // Attempt to find the AML object for the timer
    lai_variable_t pnp_id = {};
    lai_eisaid(&pnp_id, x86_64::hpet::hpet_pnp_id);
    
    LAI_CLEANUP_STATE lai_state_t state;
    lai_init_state(&state);

    lai_nsnode_t *handle = nullptr;
    struct lai_ns_iterator iter = LAI_NS_ITERATOR_INITIALIZER;

    lai_nsnode_t *node;
    while ((node = lai_ns_iterate(&iter))) {
        if(lai_check_device_pnp_id(node, &pnp_id, &state)) continue; // This aint it chief

        lai_nsnode_t* hpet_uid_node = lai_resolve_path(node, "_UID");

        int status = 1;
        lai_variable_t hpet_uid = {};
        if(hpet_uid_node != nullptr){
            status = lai_eval(&hpet_uid, hpet_uid_node, &state);
        }

        #ifdef DEBUG
        if(status == 0){
            uint64_t uid = 0xFFFFFFFFFFFFFFFF;
            lai_obj_get_integer(&hpet_uid, &uid);
            debug_printf("[HPET]: Found HPET in AML code, UID: %x\n", uid);
        } else {
            debug_printf("[HPET]: Found HPET in AML code\n");
        }
        #endif

        if(status == 0){ // Found a _UID
            // We only support one HPET so look for UID 0
            uint64_t uid = 0xFFFFFFFFFFFFFFFF;
            lai_obj_get_integer(&hpet_uid, &uid);
            if(uid == 0){
                handle = node;
                break;
            }
        } else {
            // Assume this is 0, since there is no _UID we don't know for sure
            handle = node;
            break;
        }
    }

    if(!handle){
        PANIC("[HPET]: Couldn't find HPET timer in AML");
    }
    
    acpi_table = reinterpret_cast<x86_64::hpet::table*>(acpi::get_table("HPET"));
    if(acpi_table == nullptr){
        // Huh thats odd, there is an AML device but no table?
        // TODO: In this case get the HPET base from the AML _CRS object

        lai_nsnode_t* crs_node = lai_resolve_path(node, "_CRS");
        if(!crs_node)
            PANIC("No HPET table and no _CRS, can't initialize HPET");

        LAI_CLEANUP_VAR lai_variable_t crs = {};
        if(lai_eval(&crs, crs_node, &state))
            PANIC("Couldn't evaluate HPET _CRS");

        lai_resource_view view = LAI_RESOURCE_VIEW_INITIALIZER(&crs);
        lai_api_error_t e;
        while(!(e = lai_resource_iterate(&view))) {
            enum lai_resource_type type = lai_resource_get_type(&view);
            switch(type) {
                case LAI_RESOURCE_MEM:
                    base = view.base;
                    debug_printf("[HPET]: Found HPET base in _CRS, base: %x\n", base);
                default:
                    break;
            }
        }
    } else {
        base = acpi_table->base_addr_low.address;

        if(acpi_table->base_addr_low.id != acpi::generic_address_structure_id_system_memory) {
            printf("[HPET]: Unkown ACPI Generic Address Structure Access ID: %x\n", acpi_table->base_addr_low.id);
            PANIC("");
        }
    }

    if(!base)
        PANIC("[HPET]: Wasn't able to find HPET base");

    mm::vmm::kernel_vmm::get_instance().map_page(base, (base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_writable | map_page_flags_global | map_page_flags_no_execute, map_page_chache_types::uncacheable);
    
    if(((hpet_read(general_capabilities_and_id_reg) >> 16) & 0xFFFF) == 0xFFFF)
        PANIC("[HPET]: No HPET detected, need HPET to further boot");

    #ifdef DEBUG
    debug_printf("[HPET]: Initializing HPET device: base: %x, PCI Vendor ID: %x, Revision: %x, Counter clk: %x, N counters: %x", base, ((hpet_read(general_capabilities_and_id_reg) >> 16) & 0xFFFF), (hpet_read(general_capabilities_and_id_reg) & 0xFF), main_counter_clk, n_counters);
    if(supports_64bit_counter) debug_printf(", Supports 64bit counter\n");
    else debug_printf("\n");
    #endif

    uint64_t general_cap_and_id = hpet_read(x86_64::hpet::general_capabilities_and_id_reg);
    main_counter_clk = ((general_cap_and_id >> 32) & 0xFFFFFFFF);
    if(!(main_counter_clk <= 0x05F5E100) || main_counter_clk == 0){
        printf("[HPET]: Invalid HPET COUNTER_CLK_PERIOD: %x\n", main_counter_clk);
        PANIC("");
    }

    n_counters = ((general_cap_and_id >> 8) & 0xF) + 1;
    if((general_cap_and_id & 0xFF) == 0){
        printf("[HPET]: Invalid HPET Revision: %x\n", (general_cap_and_id & 0xFF));
        PANIC("");
    }

    supports_64bit_counter = bitops<uint64_t>::bit_test(general_cap_and_id, 13);

    


    hpet_write(x86_64::hpet::general_configuration_reg, 0); // Main counter disabled, legacy routing disabled

    hpet_write(x86_64::hpet::main_counter_reg, 0); // Counter counts up

    // Initialize Timers

    hpet_write(x86_64::hpet::general_configuration_reg, 1); // Main counter enabled, legacy routing disabled

}

bool x86_64::hpet::create_timer(uint8_t comparator, x86_64::hpet::hpet_timer_types type, 
								MAYBE_UNUSED_ATTRIBUTE uint8_t vector, uint64_t ms) {
	if(comparator > n_counters) {
		printf("[HPET]: Unexisting comparator\n");
		return false;
	}

    uint64_t cap = hpet_read(hpet_timer_configuration_and_capabilities_reg(comparator));

    if(!bitops<uint64_t>::bit_test(cap, 4) && type == x86_64::hpet::hpet_timer_types::PERIODIC){
        printf("[HPET]: Comparator doesn't support Periodic mode\n");
        return false;
    }

    cap |= ((1 << 2) | (((type == x86_64::hpet::hpet_timer_types::PERIODIC) ? (1) : (0)) << 3) | (((type == x86_64::hpet::hpet_timer_types::PERIODIC) ? (1) : (0)) << 6));

    hpet_write(hpet_timer_configuration_and_capabilities_reg(comparator), cap);

    uint64_t ticks = ms / (main_counter_clk / x86_64::hpet::femto_per_milli);

    hpet_write(hpet_timer_comparator_value_reg(comparator), hpet_read(x86_64::hpet::main_counter_reg) + ticks);
    hpet_write(hpet_timer_comparator_value_reg(comparator), ticks);

	// TODO: Initialize interrupt

    return true;
}

void x86_64::hpet::poll_sleep(uint64_t ms){
    uint64_t goal = hpet_read(x86_64::hpet::main_counter_reg) + ms * x86_64::hpet::femto_per_milli / main_counter_clk;

    while(hpet_read(x86_64::hpet::main_counter_reg) < goal) asm("pause");
}