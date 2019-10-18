#include <libsigma/klog.h>
#include <libsigma/ipc.h>
#include <libsigma/thread.h>
#include <memory>
#include <iostream>
#include <Zeta/devfs.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

template<typename Ret>
void send_return_int([[maybe_unused]] uint64_t dest, [[maybe_unused]] uint64_t msg_id, [[maybe_unused]] Ret ret){
    libsigma_klog("[ZETA]: Unsupported return type");
    return;
}

template<>
void send_return_int<uint64_t>(uint64_t dest, uint64_t msg_id, uint64_t ret){
    auto ret_msg = std::make_unique<libsigma_ret_message>();

    ret_msg->command = RET;
    ret_msg->msg_id = msg_id;
    ret_msg->ret = ret; // TODO support other things than int

    libsigma_ipc_set_message_checksum(ret_msg->msg(), sizeof(libsigma_ret_message));

    libsigma_ipc_send(dest, sizeof(libsigma_ret_message), ret_msg->data());
}

template<>
void send_return_int<int>(uint64_t dest, uint64_t msg_id, int ret){
    auto ret_msg = std::make_unique<libsigma_ret_message>();

    ret_msg->command = RET;
    ret_msg->msg_id = msg_id;
    ret_msg->ret = ret; // TODO support other things than int

    libsigma_ipc_set_message_checksum(ret_msg->msg(), sizeof(libsigma_ret_message));

    libsigma_ipc_send(dest, sizeof(libsigma_ret_message), ret_msg->data());
}

template<>
void send_return_int<std::vector<char>>(uint64_t dest, uint64_t msg_id, std::vector<char> ret){
    auto* ret_msg = reinterpret_cast<libsigma_ret_message*>(new uint8_t[sizeof(libsigma_ret_message) + ret.size()]);

    ret_msg->command = RET;
    ret_msg->msg_id = msg_id;
    ret_msg->ret = ret.size();
    memcpy(ret_msg->buf, ret.data(), ret.size());

    libsigma_ipc_set_message_checksum(ret_msg->msg(), sizeof(libsigma_ret_message));

    libsigma_ipc_send(dest, sizeof(libsigma_ret_message), ret_msg->data());
}

void handle_request(){
    if(libsigma_ipc_get_msg_size() == 0) // No new message so block until there is
        libsigma_block_thread(SIGMA_BLOCK_WAITING_FOR_IPC);

    auto msg_size = libsigma_ipc_get_msg_size();
    auto msg = std::make_unique<uint8_t[]>(msg_size);
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

    auto send_return = [&](auto ret){
        auto* message = (libsigma_message_t*)msg.get();

        send_return_int(origin, message->msg_id, ret);
    };

    switch (message->command)
    {
    case OPEN:{
            auto* open_info = (libsigma_open_message*)msg.get();
            if(msg_size < open_info->path_len){ // TODO: Incoroperate the other members in this calculation
                // Wut? No we won't corrupt heap for you
                send_return(-1);
            } else {
                int ret = fs::get_vfs().open(origin, std::string_view{open_info->path, open_info->path_len}, open_info->flags);
                send_return(ret);
            }
        }
        break;
    case CLOSE:{
            auto* close_info = (libsigma_close_message*)msg.get();
            int ret = fs::get_vfs().close(origin, close_info->fd);
            send_return(ret);
        }
        break;
    case READ: {
            auto* read_info = (libsigma_read_message*)msg.get();
            std::vector<char> buf{};
            buf.resize(read_info->count);
            int ret = fs::get_vfs().read(origin, read_info->fd , static_cast<void*>(buf.data()), read_info->count);
            send_return(buf);
        }
        break;
    case WRITE: {
            auto* write_info = (libsigma_write_message*)msg.get();
            if(msg_size < write_info->count){ // TODO: Incoroperate the other members in this calculation
                // Wut? No we won't corrupt heap for you
                send_return(-1);
            } else {
                int ret = fs::get_vfs().write(origin, write_info->fd, write_info->buf, write_info->count);
                send_return(ret);
            }
        }
        break;
    case DUP2: {
            auto* dup_info = (libsigma_dup2_message*)msg.get();         
            int ret = fs::get_vfs().dup2(origin, dup_info->oldfd, dup_info->newfd);
            send_return(ret);
        }
        break;
    case SEEK: {
            auto* seek_info = (libsigma_seek_message*)msg.get();
            uint64_t useless;
            int ret = fs::get_vfs().seek(origin, seek_info->fd, seek_info->offset, seek_info->whence, useless);
            send_return(ret);
        }
        break;
    case TELL: {
            auto* tell_info = (libsigma_tell_message*)msg.get();
            uint64_t ret = fs::get_vfs().tell(origin, tell_info->fd);
            send_return(ret);
        }
        break;
    default:
        std::cout << "Unknown IPC command: 0x" << std::hex << message->command << std::endl;
        break;
    }
}


int main(){
    libsigma_klog("[ZETA]: Starting VFS");
    
    fs::devfs devfs{};
    devfs.init();
    devfs.add_file("/sysout", fs::devfs::sysout_node_factory());

    auto loop = [&](){
        while(true) handle_request();
    };
    loop();

    while(true);
}