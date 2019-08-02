#ifndef SIGMA_KERNEL_ACPI_FADT
#define SIGMA_KERNEL_ACPI_FADT

#include <Sigma/common.h>

#include <Sigma/acpi/tables.h>

namespace acpi
{
    constexpr const char* fadt_signature = "FACP";

    struct PACKED_ATTRIBUTE fadt {
        acpi::sdt_header header;
        uint32_t firmware_control;
        uint32_t dsdt;
        uint8_t reserved_0;
        uint8_t preferred_pm_profile;
        uint16_t sci_int;
        uint32_t sci_cmd;
        uint8_t acpi_enable;
        uint8_t acpi_disable;
        uint8_t s4bios_req;
        uint8_t pstate_cnt;
        uint32_t pm1a_evt_blk;
        uint32_t pm1b_evt_blk;
        uint32_t pm1a_cnt_blk;
        uint32_t pm1b_cnt_blk;
        uint32_t pm2_cnt_blk;
        uint32_t pm_tmr_blk;
        uint32_t gpe0_blk;
        uint32_t gpe1_blk;
        uint8_t pm1_evt_len;
        uint8_t pm1_cnt_len;
        uint8_t pm2_cnt_len;
        uint8_t pm_tmr_len;
        uint8_t gpe0_blk_len;
        uint8_t gpe1_blk_len;
        uint8_t gpe1_base;
        uint8_t cst_cnt;
        uint16_t p_lvl2_lat;
        uint16_t p_lvl3_lat;
        uint16_t flush_size;
        uint16_t flush_stride;
        uint8_t duty_offset;
        uint8_t duty_width;
        uint8_t day_alarm;
        uint8_t month_alarm;
        uint8_t century;
        uint16_t iapc_boot_arch;
        uint8_t reserved_1;
        uint32_t flags;
        acpi::generic_address_structure reset_reg;
        uint8_t reset_value;
        uint16_t arm_boot_arch;
        uint8_t minor_version;
        uint64_t x_firmware_ctrl;
        uint64_t x_dsdt;
        acpi::generic_address_structure x_pm1a_evt_blk;
        acpi::generic_address_structure x_pm1b_evt_blk;
        acpi::generic_address_structure x_pm1a_cnt_blk;
        acpi::generic_address_structure x_pm1b_cnt_blk;
        acpi::generic_address_structure x_pm2_cnt_blk;
        acpi::generic_address_structure x_pm_tmr_blk;
        acpi::generic_address_structure x_gpe0_blk;
        acpi::generic_address_structure x_gpe1_blk;
        acpi::generic_address_structure sleep_control_reg;
        acpi::generic_address_structure sleep_status_reg;
        uint64_t hypervisor_vendor_identity;
    };

    constexpr uint8_t preferred_pm_profile_unspecified = 0;
    constexpr uint8_t preferred_pm_profile_desktop = 1;
    constexpr uint8_t preferred_pm_profile_mobile = 2;
    constexpr uint8_t preferred_pm_profile_workstation = 3;
    constexpr uint8_t preferred_pm_profile_enterprise_server = 4;
    constexpr uint8_t preferred_pm_profile_soho_server = 5;
    constexpr uint8_t preferred_pm_profile_appliance_pc = 6;
    constexpr uint8_t preferred_pm_profile_performance_server = 7;
    constexpr uint8_t preferred_pm_profile_tablet = 8;

    constexpr uint16_t iapc_boot_arch_legacy_devices = 0;
    constexpr uint16_t iapc_boot_arch_8042 = 1;
    constexpr uint16_t iapc_boot_arch_vga_not_present = 2;
    constexpr uint16_t iapc_boot_arch_msi_not_supported = 3;
    constexpr uint16_t iapc_boot_arch_pcie_aspm = 4;
    constexpr uint16_t iapc_boot_arch_no_cmos_rtc = 5;


    constexpr uint16_t flags_wbinvd = 0;
    constexpr uint16_t flags_wbinvd_flush = 1;
    constexpr uint16_t flags_proc_c1 = 2;
    constexpr uint16_t flags_p_lvl2_up = 3;
    constexpr uint16_t flags_pwr_button = 4;
    constexpr uint16_t flags_slp_button = 5;
    constexpr uint16_t flags_fix_rtc = 6;
    constexpr uint16_t flags_rtc_s4 = 7;
    constexpr uint16_t flags_tmr_val_ext = 8;
    constexpr uint16_t flags_dck_cap = 9;
    constexpr uint16_t flags_reset_reg_sup = 10;
    constexpr uint16_t flags_sealed_case = 11;
    constexpr uint16_t flags_headless = 12;
    constexpr uint16_t flags_cpu_sw_slp = 13;
    constexpr uint16_t flags_pci_ext_wak = 14;
    constexpr uint16_t flags_use_platform_clock = 15;
    constexpr uint16_t flags_s4_rtc_sts_valid = 16;
    constexpr uint16_t flags_remote_power_on_capable = 17;
    constexpr uint16_t flags_force_apic_cluster_model = 18;
    constexpr uint16_t flags_force_apic_physical_destination_mode = 19;
    constexpr uint16_t flags_hw_reduced_acpi = 20;
    constexpr uint16_t flags_low_power_s0_idle_capable = 21;

    constexpr uint16_t arm_boot_arch_psci_compliant = 0;
    constexpr uint16_t arm_boot_arch_psci_use_hvc = 1;
} // namespace acpi

#endif