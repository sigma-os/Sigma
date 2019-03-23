#pragma once
#include <efi.h>
#include <efilib.h>
#include <sigma_loader.h>

void init_sigma_graphics();
void sigma_set_video_mode(uint64_t width, uint64_t height);
VOID* sigma_get_framebuffer();