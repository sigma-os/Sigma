#include <libsigma/klog.h>
#include <libsigma/ipc.h>
#include <libsigma/thread.h>
#include <memory>
#include <iostream>
#include <Zeta/devfs.h>
#include <unistd.h>
#include <stdio.h>

template<typename Ret>
void send_return(uint64_t dest, uint64_t msg_id, Ret ret){
    auto* ret_msg = new libsigma_ret_message;

    ret_msg->command = RET;
    ret_msg->msg_id = msg_id;
    ret_msg->ret = ret; // TODO support other things than int
    libsigma_ipc_set_message_checksum((libsigma_message*)ret_msg, sizeof(libsigma_ret_message));

    libsigma_ipc_send(dest, sizeof(libsigma_ret_message), reinterpret_cast<uint8_t*>(ret_msg));

    delete ret_msg;
}

void handle_request(){
    while(libsigma_ipc_get_msg_size() == 0); // 0 = No message

    auto msg_size = libsigma_ipc_get_msg_size();
    auto msg = std::make_unique<uint8_t>(msg_size);
    uint64_t origin;
    size_t useless_msg_size;
    if(libsigma_ipc_receive(&origin, &useless_msg_size, msg.get()) == 1){
        libsigma_klog("[ZETA]: Failed to receive IPC message");
        return;
    }

    auto* message = reinterpret_cast<libsigma_message_t*>(msg.get());

    if(!libsigma_ipc_check_message(message, msg_size)) {
        libsigma_klog("[ZETA]: Checksum failed");
        return;
    }

    switch (message->command)
    {
    case OPEN:{
            auto* open_info = (libsigma_open_message*)msg.get();
            if(msg_size < open_info->path_len){ // TODO: Incoroperate the other members in this calculation
                // Wut? No we won't corrupt heap for you
                send_return<int>(origin, open_info->msg_id, -1);
            } else {
                int ret = fs::get_vfs().open(origin, std::string_view{open_info->path, open_info->path_len}, open_info->flags);
                send_return<int>(origin, open_info->msg_id, ret);
            }
        }
        break;
    case WRITE: {
            auto* write_info = (libsigma_write_message*)msg.get();
            if(msg_size < write_info->count){ // TODO: Incoroperate the other members in this calculation
                // Wut? No we won't corrupt heap for you
                send_return<int>(origin, write_info->msg_id, -1);
            } else {
                int ret = fs::get_vfs().write(origin, write_info->fd, write_info->buf, write_info->count);
                send_return<int>(origin, write_info->msg_id, ret);
            }
        }
        break;
    case DUP2: {
            auto* dup_info = (libsigma_dup2_message*)msg.get();
            int ret = fs::get_vfs().dup2(origin, dup_info->oldfd, dup_info->newfd);
            send_return<int>(origin, dup_info->msg_id, ret);
        }
        break;
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




    //int fd = fs::get_vfs().open(1, "/dev/sysout", 0);
    /*char* yes = "[ZETA]: Hello from /dev/sysout";
    fs::get_vfs().write(1, fd, static_cast<void*>(yes), 31);*/

    //fs::get_vfs().dup2(1, fd, STDOUT_FILENO);

    //printf("[ZETA]: Hello from printf");

    auto loop = [&](){
        while(true) handle_request();
    };
    loop();

    while(true);
}