#include <stdio.h>
#include <fcntl.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <cassert>
#include <libsigma/sys.h>
#include <memory>
#include <protocols/kbus.hpp>
#include <sys/auxv.h>

#include <fcntl.h>
#include <unistd.h>

using object_id = uint64_t;

struct object {
    std::unordered_map<std::string, std::string> attributes;
    object_id id;
};

std::unordered_map<object_id, object> objects;
object_id id_generator = 0;

void handle_request(){
    if(libsigma_ipc_get_msg_size() == 0) // No new message so block until there is
        libsigma_block_thread(SIGMA_BLOCK_WAITING_FOR_IPC);

    auto msg_size = libsigma_ipc_get_msg_size();
    auto msg_raw = std::make_unique<uint8_t[]>(msg_size);
    auto* msg = (libsigma_message_t*)msg_raw.get();

    tid_t origin;
    size_t useless_msg_size;
    if(libsigma_ipc_receive(&origin, msg, &useless_msg_size) == 1){
        printf("kbus: Failed to receive IPC message\n");
        return;
    }

    sigma::kbus::client_request_parser parser{msg_raw.get(), msg_size};
    assert(parser.has_command());

    auto command = static_cast<sigma::kbus::client_request_type>(parser.get_command());

    auto send_return = [origin](sigma::kbus::server_response_builder& res){
        auto* buf = res.serialize();
        size_t len = res.length();
        libsigma_ipc_send(origin, (libsigma_message_t*)buf, len);
    };

    switch (command) {
    case sigma::kbus::client_request_type::CreateDevice: {
            object_id id = id_generator++;

            sigma::kbus::server_response_builder builder{};
            builder.add_status((uint64_t)sigma::kbus::server_response_status::Success);
            builder.add_device(id);

            send_return(builder);
        }
        break;
    case sigma::kbus::client_request_type::AddAttribute: {
            assert(parser.has_key());
            assert(parser.has_value());
            assert(parser.has_device());

            objects[parser.get_device()].attributes[parser.get_key()] = parser.get_value();

            sigma::kbus::server_response_builder builder{};
            builder.add_status((uint64_t)sigma::kbus::server_response_status::Success);
            
            send_return(builder);
        }
        break;
    case sigma::kbus::client_request_type::GetAttribute: {
            assert(parser.has_key());
            assert(parser.has_device());

            auto& attrib = objects[parser.get_device()].attributes[parser.get_key()];

            sigma::kbus::server_response_builder builder{};
            builder.add_status((uint64_t)sigma::kbus::server_response_status::Success);
            builder.add_value(attrib);
            
            
            send_return(builder);
        }
        break;
    case sigma::kbus::client_request_type::FindDevices: {
            assert(parser.has_query());

            auto query = parser.get_query();
            query.erase(std::remove_if(query.begin(), query.end(), isspace), query.end()); // Remove whitespace

            auto string_split = [](std::string str, char seperator) -> std::vector<std::string_view> {
                auto ret = std::vector<std::string_view>{};
                for(size_t i = 0; i < str.length(); i++){
                    char c = str[i];
                    if(c == seperator){
                        size_t j;
                        for(j = i + 1; j < str.length(); j++)
                            if(str[j] == seperator)
                            break;
            
                        size_t size = j - i;
                        if(size != 1)
                            ret.push_back(std::string_view{str}.substr(i + 1, size - 1));
                    }
                }

                return ret;
            };

            auto conditions = string_split(query, '&');

            std::vector<object_id> ids{};

            for(auto& [id, entry] : objects){
                for(auto& condition : conditions){
                    auto split = string_split(std::string{condition}, '=');
                    assert(split.size() == 2);

                    auto& lhs = split[0];
                    auto& rhs = split[1];
                    if(entry.attributes[std::string{lhs}] != rhs)
                        continue;
                }

                ids.push_back(id);
            }

            sigma::kbus::server_response_builder builder{};
            builder.add_status((uint64_t)sigma::kbus::server_response_status::Success);
            builder.add_results(ids);
            
            send_return(builder);
        }
        break;
    default:
        printf("kbus: Unknown command: %ld\n", parser.get_command());
        break;
    }
}

int main(){    
    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("kbus: Starting\n");
    while(true)
        handle_request();
}