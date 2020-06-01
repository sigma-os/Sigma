#include <libsigma/sys.h>
#include <protocols/zeta-std.hpp>
#include <memory>
#include <iostream>
#include <Zeta/devfs.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <atomic>
#include <experimental/coroutine>
#include <async/basic.hpp>
#include <async/result.hpp>

/*#define ZETA_ASSERT(condition) do { \
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

async::result<void> handle_requests(){
    libsigma_klog("zeta: Handling requests\n");
    while(true) {
        if(libsigma_ipc_get_msg_size() == 0) // No new message so block until there is
            libsigma_block_thread(SIGMA_BLOCK_WAITING_FOR_IPC);

        auto msg_size = libsigma_ipc_get_msg_size();
        auto msg_raw = std::make_unique<uint8_t[]>(msg_size);
        auto* msg = (libsigma_message_t*)msg_raw.get();

        tid_t origin;
        size_t useless_msg_size;
        if(libsigma_ipc_receive(&origin, msg, &useless_msg_size) == 1){
            libsigma_klog("zeta: Failed to receive IPC message\n");
            break;
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
}


struct sigma_io_service : async::io_service {
    sigma_io_service() {}

    sigma_io_service(const sigma_io_service &) = delete;
	
	sigma_io_service &operator= (const sigma_io_service &) = delete;

    void wait() override {
        libsigma_yield();
    }

    static sigma_io_service& global();
};

sigma_io_service& sigma_io_service::global() {
	static sigma_io_service service;
	return service;
}

async::run_queue *globalQueue() {
	static async::run_queue queue{&sigma_io_service::global()};
	return &queue;
}

async::detached init(){
    async::detach(handle_requests());
}*/

int main(){
    fs::devfs devfs{};
    devfs.init();

    libsigma_klog("zeta: Started VFS\n");
    
    /*{
        async::queue_scope scope{globalQueue()};
        init();
    }

    globalQueue()->run();*/
}