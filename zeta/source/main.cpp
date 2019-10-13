#include <libsigma/klog.h>
#include <libsigma/ipc.h>
#include <libsigma/thread.h>
#include <memory>
#include <iostream>
#include <Zeta/devfs.h>

void handle_request(){
    while(libsigma_ipc_get_msg_size() == 0); // 0 = No message

    auto msg_size = libsigma_ipc_get_msg_size();
    auto msg = std::make_unique<uint8_t>(msg_size);
    uint64_t origin;
    size_t useless_msg_size;
    libsigma_ipc_receive(&origin, &useless_msg_size, msg.get());

    auto* message = reinterpret_cast<libsigma_message_t*>(msg.get());

    if(!libsigma_ipc_check_message(message, msg_size)) return;

    switch (message->command)
    {
    default:
        std::cout << "Unknown IPC command: 0x" << std::hex << message->command << std::endl;
        break;
    }
}


int main(){
    libsigma_klog("[ZETA]: Starting VFS");

    // TODO: VFS and shit
    
    fs::devfs devfs{};
    devfs.init();
    devfs.add_file("/sysout", fs::devfs::create_sysout_node());
    // Setupped /dev/sysout




    int fd = fs::get_vfs().open(1, "/dev/sysout", 0);
    char* yes = "[ZETA]: Hello from /dev/sysout";
    fs::get_vfs().write(1, fd, static_cast<void*>(yes), 31);

    /*auto loop = [&](){
        while(true) handle_request();
    };
    loop();*/

    while(true);
}