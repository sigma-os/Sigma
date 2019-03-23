#include <sigma_file.h>

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* simple_file_system;
EFI_FILE_PROTOCOL* root = 0;
EFI_SYSTEM_TABLE* st;


void init_sigma_file(){
    st = get_system_table();
    EFI_STATUS status;
    status = uefi_call_wrapper(st->BootServices->LocateProtocol, 3, &gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&simple_file_system);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d", __FILE__, __LINE__, status);
        return;
    }
    status = uefi_call_wrapper(simple_file_system->OpenVolume, 2, simple_file_system, &root);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d", __FILE__, __LINE__, status);
        return;
    }


}
sigma_file_t* sigma_open_file(CHAR16* name){
    EFI_FILE_HANDLE* file;
    EFI_STATUS status;
    
    status = uefi_call_wrapper(root->Open, 5, root, &file, name, EFI_FILE_MODE_READ, 0);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d", __FILE__, __LINE__, status);
        return NULL;
    }

    return (sigma_file_t*)file;
}
UINTN sigma_read_file(sigma_file_t* file, UINTN offset, UINTN size, uint8_t* buf){
    EFI_FILE_HANDLE* efi_file = (EFI_FILE_HANDLE*)file;

    EFI_STATUS status;

    status = uefi_call_wrapper(root->SetPosition, 2, efi_file, offset);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d", __FILE__, __LINE__, status);
        return 0;
    }

    
    status = uefi_call_wrapper(root->Read, 3, efi_file, &size, (void*)buf);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d", __FILE__, __LINE__, status);
        return 0;
    }

    return size;
}
void sigma_close_file(sigma_file_t* file){
    EFI_FILE_HANDLE* efi_file = (EFI_FILE_HANDLE*)file;

    EFI_STATUS status;

    status = uefi_call_wrapper(root->Close, 1, efi_file);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d", __FILE__, __LINE__, status);
        return;
    }
}

UINTN sigma_get_file_size(sigma_file_t* file){
    EFI_FILE_HANDLE* efi_file = (EFI_FILE_HANDLE*)file;

    EFI_STATUS status;
    UINTN size = sizeof(EFI_FILE_INFO) + 100;
    void* buf = AllocatePool(size);
    EFI_GUID g = EFI_FILE_INFO_ID;
    
    status = uefi_call_wrapper(root->GetInfo, 4, efi_file, &g, &size, buf);
    if(EFI_ERROR(status)){
        Print(L"%s:%d: EFI ERROR %d", __FILE__, __LINE__, status);
        return 0;
    }

    EFI_FILE_INFO* f = buf;
    return f->FileSize;
}