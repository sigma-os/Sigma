#pragma once
#include <efi/efi.h>
#include <efi/efilib.h>
#include <stdint.h>

typedef struct {
    uint64_t* mmap;
    uint64_t* mmap_end;
    uint64_t memorySize;
    uint64_t* acpiPointer;
} sigma_booted_header_t;

EFI_SYSTEM_TABLE* get_system_table();