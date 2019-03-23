#pragma once
#include <stdint.h>
#include <efi.h>
#include <efilib.h>
#include <sigma_loader.h>

typedef EFI_FILE_HANDLE sigma_file_t;

void init_sigma_file();
sigma_file_t* sigma_open_file(CHAR16* name);
UINTN sigma_read_file(sigma_file_t* file, UINTN offset, UINTN size, uint8_t* buf);
void sigma_close_file(sigma_file_t* file);