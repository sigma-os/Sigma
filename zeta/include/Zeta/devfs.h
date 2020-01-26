#pragma once

#include <Zeta/vfs.h>
#include <libsigma/klog.h>
#include <cstring>
#include <string>
#include <unordered_map>

namespace fs
{
    class devfs {
        public:
        void init(){
            fs_calls calls{};
            calls.open = [&, this](fs_node** node_out, const char* str, [[maybe_unused]] int mode) -> int {
                std::string tmp{str};

                #if 0 // This works for C++20 but not C++17
	            if(!this->files.contains(fs_type)){
                    *node_out = nullptr;
                    return -1;
                }
	            #else // C++17
	            if(this->files.find(tmp) == this->files.end()){
                    *node_out = nullptr;
                    return -1;
                }
	            #endif

                // Exists in the map
                *node_out = this->files[tmp];
                return 0;
            };
            
            fs::get_vfs().register_fs("devfs", calls);

            auto* node = new fs::fs_node{};
            node->type = fs::fs_node_types::mountpoint;
            node->path = "/dev";

            fs::get_vfs().mount(node, "/dev", "devfs");

            add_file("/kmsg", kmsg_node_factory());
            add_file("/null", null_node_factory());
            add_file("/zero", zero_node_factory());
            add_file("/full", full_node_factory());

            // TODO: Actually implement terminals and shit and not just redirect std{out, err} to kmsg
            add_file("/stdout", kmsg_node_factory());
            add_file("/stderr", kmsg_node_factory());
        }

        void add_file(std::string path, fs::fs_node* node){
            node->path = path;
            files[path] = node;
        }

        static fs::fs_node* kmsg_node_factory(){
            auto* node = new fs::fs_node{};
            node->path = "/kmesg";
            node->type = fs::fs_node_types::block_device;
            node->calls.write = []([[maybe_unused]] fs_node* node, const void* buf, size_t size, [[maybe_unused]] size_t offset) -> int{
                std::string str{static_cast<const char*>(buf), size};
                libsigma_klog(str.c_str());

                return 0;
            };
            return node;
        }

        static fs::fs_node* null_node_factory(){
            auto* node = new fs::fs_node{};
            node->path = "/null";
            node->type = fs::fs_node_types::block_device;
            node->calls.write = []([[maybe_unused]] fs_node* node, [[maybe_unused]] const void* buf, [[maybe_unused]] size_t size, [[maybe_unused]] size_t offset) -> int{
                // Writes Ignored
                return 0;
            };

            node->calls.read = []([[maybe_unused]] fs_node* node, void* buf, size_t size, size_t offset) -> int {
                memset(buf, EOF, size); // Reads return EOF
                return 0;
            };
            
            return node;
        }

        static fs::fs_node* zero_node_factory(){
            auto* node = new fs::fs_node{};
            node->path = "/zero";
            node->type = fs::fs_node_types::block_device;
            node->calls.write = []([[maybe_unused]] fs_node* node, [[maybe_unused]] const void* buf, [[maybe_unused]] size_t size, [[maybe_unused]] size_t offset) -> int{
                // Writes Ignored
                return 0;
            };

            node->calls.read = []([[maybe_unused]] fs_node* node, void* buf, size_t size, size_t offset) -> int {
                memset(buf, 0, size); // Reads return 0
                return 0;
            };
            
            return node;
        }

        static fs::fs_node* full_node_factory(){
            auto* node = new fs::fs_node{};
            node->path = "/full";
            node->type = fs::fs_node_types::block_device;
            node->calls.write = []([[maybe_unused]] fs_node* node, [[maybe_unused]] const void* buf, [[maybe_unused]] size_t size, [[maybe_unused]] size_t offset) -> int{
                return ENOSPC;
            };

            node->calls.read = []([[maybe_unused]] fs_node* node, void* buf, size_t size, size_t offset) -> int {
                memset(buf, 0, size); // Reads return 0
                return 0;
            };
            
            return node;
        }

        std::unordered_map<std::string, fs::fs_node*> files;
    };
} // namespace devfs