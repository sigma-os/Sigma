#include <kbus/kbus.hpp>
#include <libsigma/sys.h>
#include <stdio.h>
#include <protocols/kbus.hpp>
#include <cassert>
#include <vector>
#include <sys/auxv.h>

#define CHECK_VALUE(val, ret)  do { \
                                if(!parser.has_##val()){ \
                                    printf("libkbus: response does not have" #val "\n"); \
                                    return ret; \
                                } \
                            } while(0)

tid_t kbus_ring = 0;

static handle_t get_kbus_ring(){
    if(kbus_ring == 0)
        kbus_ring = getauxval(AT_KBUS_SERVER);

    return kbus_ring;
}

kbus::object_id kbus::allocate_object(){
    using namespace sigma::kbus;
    client_request_builder builder{};

    builder.add_command((uint64_t)client_request_type::CreateDevice);

    if(libsigma_ipc_send(get_kbus_ring(), (libsigma_message_t*)builder.serialize(), builder.length())){
        printf("libkbus: Failed to send CreateDevice message\n");
        return -1;
    }

    libsigma_block_thread(SIGMA_BLOCK_WAITING_FOR_IPC);

    size_t res_size = libsigma_ipc_get_msg_size(get_kbus_ring());
    std::vector<uint8_t> res{};
    res.resize(res_size);

    if(libsigma_ipc_receive(get_kbus_ring(), (libsigma_message_t*)res.data())){
        printf("libkbus: Failed to receive CreateDevice response\n");
        return -1;
    }

    server_response_parser parser{res.data(), res.size()};

    CHECK_VALUE(status, -1);
    CHECK_VALUE(device, -1);

    assert((server_response_status)parser.get_status() == server_response_status::Success);

    return parser.get_device();
}

std::vector<kbus::object> kbus::find_devices(std::string query){
    using namespace sigma::kbus;
    client_request_builder builder{};

    builder.add_command((uint64_t)client_request_type::FindDevices);
    builder.add_query(query);

    if(libsigma_ipc_send(get_kbus_ring(), (libsigma_message_t*)builder.serialize(), builder.length())){
        printf("libkbus: Failed to send FindDevices message\n");
        return {};
    }

    libsigma_block_thread(SIGMA_BLOCK_WAITING_FOR_IPC);

    size_t res_size = libsigma_ipc_get_msg_size(get_kbus_ring());
    std::vector<uint8_t> res{};
    res.resize(res_size);

    if(libsigma_ipc_receive(get_kbus_ring(), (libsigma_message_t*)res.data())){
        printf("libkbus: Failed to receive FindDevices response\n");
        return {};
    }

    server_response_parser parser{res.data(), res.size()};

    CHECK_VALUE(status, {});
    CHECK_VALUE(results, {});

    assert((server_response_status)parser.get_status() == server_response_status::Success);

    std::vector<kbus::object> ret{};
    const auto& results = parser.get_results();
    for(const auto id : results)
        ret.emplace_back(id);

    return ret;
}

kbus::object::object(): object{kbus::allocate_object()} {}
kbus::object::object(kbus::object_id id): id{id} {}

std::string kbus::object::get_attribute(std::string key){
    using namespace sigma::kbus;
    client_request_builder builder{};

    builder.add_command((uint64_t)client_request_type::GetAttribute);
    builder.add_device(this->id);
    builder.add_key(key);

    if(libsigma_ipc_send(get_kbus_ring(), (libsigma_message_t*)builder.serialize(), builder.length())){
        printf("libkbus: Failed to send GetAttribute message\n");
        return "";
    }

    libsigma_block_thread(SIGMA_BLOCK_WAITING_FOR_IPC);

    size_t res_size = libsigma_ipc_get_msg_size(get_kbus_ring());
    std::vector<uint8_t> res{};
    res.resize(res_size);

    if(libsigma_ipc_receive(get_kbus_ring(), (libsigma_message_t*)res.data())){
        printf("libkbus: Failed to receive GetAttribute response\n");
        return "";
    }

    server_response_parser parser{res.data(), res.size()};

    CHECK_VALUE(status, "");
    CHECK_VALUE(value, "");

    assert((server_response_status)parser.get_status() == server_response_status::Success);

    return parser.get_value();
}

void kbus::object::set_attribute(std::string key, std::string value){
using namespace sigma::kbus;
    client_request_builder builder{};

    builder.add_command((uint64_t)client_request_type::AddAttribute);
    builder.add_device(this->id);
    builder.add_key(key);
    builder.add_value(value);

    if(libsigma_ipc_send(get_kbus_ring(), (libsigma_message_t*)builder.serialize(), builder.length())){
        printf("libkbus: Failed to send AddAttribute message\n");
        return;
    }

    if(libsigma_ipc_get_msg_size(get_kbus_ring()) == 0)
        libsigma_block_thread(SIGMA_BLOCK_WAITING_FOR_IPC);

    size_t res_size = libsigma_ipc_get_msg_size(get_kbus_ring());
    std::vector<uint8_t> res{};
    res.resize(res_size);

    if(libsigma_ipc_receive(get_kbus_ring(), (libsigma_message_t*)res.data())){
        printf("libkbus: Failed to receive AddAttribute response\n");
        return;
    }

    server_response_parser parser{res.data(), res.size()};

    CHECK_VALUE(status,);

    assert((server_response_status)parser.get_status() == server_response_status::Success);
}