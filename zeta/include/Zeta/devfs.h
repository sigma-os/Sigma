#ifndef ZETA_DEVFS
#define ZETA_DEVFS

#include <Zeta/vfs.h>
#include <libsigma/klog.h>

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
        }

        void add_file(std::string path, fs::fs_node* node){
            files[path] = node;
        }

        static fs::fs_node* sysout_node_factory(){
            auto* node = new fs::fs_node{};
            node->path = "/sysout";
            node->type = fs::fs_node_types::block_device;
            node->calls.write = []([[maybe_unused]] fs_node* node, const void* buf, size_t size, [[maybe_unused]] size_t offset) -> int{
                std::string str{static_cast<const char*>(buf), size};
                libsigma_klog(str.c_str());

                return 0;
            };
            return node;
        }

        std::unordered_map<std::string, fs::fs_node*> files;
    };
} // namespace devfs


#endif