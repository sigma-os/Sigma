#include <sigma_graphics.h>

EFI_GRAPHICS_OUTPUT_PROTOCOL* graphics;
EFI_GRAPHICS_OUTPUT_MODE_INFORMATION** modes;
UINTN* mode_sizes;


void init_sigma_graphics(){
    EFI_SYSTEM_TABLE* st = get_system_table();

    EFI_STATUS status;
    status = uefi_call_wrapper(st->BootServices->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&graphics);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d", __FILE__, __LINE__, status);
        return;
    }

    modes = AllocatePool(sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*) * (graphics->Mode->MaxMode));
    mode_sizes = AllocatePool(sizeof(UINTN) * graphics->Mode->MaxMode);

    for(UINTN i = 0; i < graphics->Mode->MaxMode; i++){
        mode_sizes[i] = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
        status = uefi_call_wrapper(graphics->QueryMode, 4, graphics, i, &(mode_sizes[i]), &(modes[i]));
        if(EFI_ERROR(status)){
            Print(L"%s:%d: EFI ERROR %d\n", __FILE__, __LINE__, status);
            return;
        }
    }

    /*for(UINTN i = 0; i < graphics->Mode->MaxMode; i++){
        Print(L"%d, %d ", modes[i]->HorizontalResolution, modes[i]->VerticalResolution);
        if((i % 4) == 0) Print(L"\n");
    }*/

    
}
void sigma_set_video_mode(uint64_t width, uint64_t height){
    EFI_STATUS status;
    for(UINTN i = 0; i < graphics->Mode->MaxMode; i++){
        if(modes[i]->HorizontalResolution == width && modes[i]->VerticalResolution == height){
            status = uefi_call_wrapper(graphics->SetMode, 2, graphics, i);
            if(EFI_ERROR(status)){
                Print(L"%s:%d: EFI ERROR %d\n", __FILE__, __LINE__, status);
                return;
            }
        }
    }

    Print(L"Couln't find graphics mode setting mode 0");
    status = uefi_call_wrapper(graphics->SetMode, 2, graphics, 0);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d\n", __FILE__, __LINE__, status);
        return;
    }
}

VOID* sigma_get_framebuffer(){
    return (VOID*)graphics->Mode->FrameBufferBase;
}