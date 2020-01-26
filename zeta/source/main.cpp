#include <libsigma/klog.h>
#include <libsigma/ipc.h>
#include <libsigma/thread.h>
#include <protocols/zeta-std.hpp>
#include <memory>
#include <iostream>
#include <Zeta/devfs.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ZETA_ASSERT(condition) do { \
				if(!(condition)){ \
                    libsigma_klog("Zeta Assertion Failed, condition: " #condition); \
                    while(1) asm("pause"); \
				} \
			} while(0);

static void send_return(tid_t target, sigma::zeta::server_response_builder& res){
    auto* buf = res.serialize();
    size_t len = res.length();
    libsigma_ipc_send(target, (libsigma_message_t*)buf, len);
}

void handle_request(){
    if(libsigma_ipc_get_msg_size() == 0) // No new message so block until there is
        libsigma_block_thread(SIGMA_BLOCK_WAITING_FOR_IPC);

    auto msg_size = libsigma_ipc_get_msg_size();
    auto msg_raw = std::make_unique<uint8_t[]>(msg_size);
    auto* msg = (libsigma_message_t*)msg_raw.get();

    tid_t origin;
    size_t useless_msg_size;
    if(libsigma_ipc_receive(&origin, msg, &useless_msg_size) == 1){
        libsigma_klog("zeta: Failed to receive IPC message\n");
        return;
    }

    sigma::zeta::client_request_parser parser{msg_raw.get(), msg_size};
    ZETA_ASSERT(parser.has_command());

    auto command = static_cast<sigma::zeta::client_request_type>(parser.get_command());

    switch (command)
    {
    case sigma::zeta::client_request_type::Open: {
            ZETA_ASSERT(parser.has_path());
            ZETA_ASSERT(parser.has_flags());

            int ret = fs::get_vfs().open(origin, std::string_view{parser.get_path()}, parser.get_flags());

            sigma::zeta::server_response_builder builder{};
            builder.add_status(0);
            builder.add_fd(ret);
            
            send_return(origin, builder);
        }
        break;
    case sigma::zeta::client_request_type::Close: {
            ZETA_ASSERT(parser.has_fd());
            int ret = fs::get_vfs().close(origin, parser.get_fd());

            sigma::zeta::server_response_builder builder{};

            builder.add_status(ret);

            send_return(origin, builder);
        }
        break;
    case sigma::zeta::client_request_type::Read: {
            ZETA_ASSERT(parser.has_fd());
            ZETA_ASSERT(parser.has_count());

            std::vector<uint8_t> buffer{};
            buffer.resize(parser.get_count());

            int ret = fs::get_vfs().read(origin, parser.get_fd() , static_cast<void*>(buffer.data()), parser.get_count());
            
            sigma::zeta::server_response_builder builder{};
            builder.add_status(ret);
            builder.add_buffer(buffer);

            send_return(origin, builder);
        }
        break;
    case sigma::zeta::client_request_type::Write: {
            ZETA_ASSERT(parser.has_buffer());
            ZETA_ASSERT(parser.has_fd());
            ZETA_ASSERT(parser.has_count());

            ZETA_ASSERT(parser.get_buffer().size() >= parser.get_count());
            
            int ret = fs::get_vfs().write(origin, parser.get_fd(), parser.get_buffer().data(), parser.get_count());
            
            sigma::zeta::server_response_builder builder{};
            builder.add_status(ret);

            send_return(origin, builder);
        }
        break;
    case sigma::zeta::client_request_type::Dup2: {
            ZETA_ASSERT(parser.has_fd());
            ZETA_ASSERT(parser.has_newfd());

            int ret = fs::get_vfs().dup2(origin, parser.get_fd(), parser.get_newfd());
            
            sigma::zeta::server_response_builder builder{};
            builder.add_status(ret);

            send_return(origin, builder);
        }
        break;
    case sigma::zeta::client_request_type::Seek: {
            ZETA_ASSERT(parser.has_offset());
            ZETA_ASSERT(parser.has_whence());
            ZETA_ASSERT(parser.has_fd());
            
            uint64_t useless;
            int ret = fs::get_vfs().seek(origin, parser.get_fd(), parser.get_offset(), parser.get_whence(), useless);
            
            sigma::zeta::server_response_builder builder{};
            builder.add_status(ret);

            send_return(origin, builder);
        }
        break;
    case sigma::zeta::client_request_type::Tell: {
            ZETA_ASSERT(parser.has_fd());

            uint64_t ret = fs::get_vfs().tell(origin, parser.get_fd());
            
            sigma::zeta::server_response_builder builder{};
            builder.add_status(0);
            builder.add_offset(ret);

            send_return(origin, builder);
        }
        break;
    default:
        break;
    }
}


int main(){
    fs::devfs devfs{};
    devfs.init();

    auto loop = [&](){
        while(true) 
            handle_request();
    };
    libsigma_klog("zeta: Started VFS\n");
    loop();

    while(true);
}