#include <efi.h>
#include <efilib.h>
#include <stdint.h>
#include <sigma_loader.h>
#include <sigma_file.h>
#include <acpi.h>


sigma_booted_header_t sigma_header;
EFI_SYSTEM_TABLE* st;

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    uefi_call_wrapper(SystemTable->BootServices->SetWatchdogTimer, 4, 0, 0, 0, NULL);
    st = SystemTable;

    init_sigma_file();


    Print(L"Booting Sigma\n");

    return EFI_SUCCESS;
}

EFI_SYSTEM_TABLE* get_system_table(){
    return st;
}