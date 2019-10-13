#ifndef ZETA_DEVFS
#define ZETA_DEVFS

#include <string>
#include <vector>
#include <cstring>
#include <utility>
#include <Zeta/vfs.h>
#include <libsigma/klog.h>
#include <cstdio>

namespace fs
{
    class devfs {
        public:
        void init(){
            fs_calls calls{};
            calls.open = [&, this](fs_node** node_out, const char* str, int mode) -> int{
                for(const auto& [path, node] : this->files){
                    if(path.compare(str) == 0){
                        *node_out = node;
                        return 0;
                    }
                }
                return 1;
            };
            
            fs::get_vfs().register_fs("devfs", calls);

            auto* node = new fs::fs_node;
            node->type = fs::fs_node_types::mountpoint;
            node->path = "/dev";

            fs::get_vfs().mount(node, "/dev", "devfs");
        }

        void add_file(std::string path, fs::fs_node* node){
            files.push_back({path, node});
        }

        static fs::fs_node* create_sysout_node(){
            auto* node = new fs::fs_node;
            node->path = "/sysout";
            
            node->calls.write = [](fs_node* node, const void* buf, size_t size, size_t offset) -> int{
                char* tmp = new char[size + 1];
                std::memset(static_cast<void*>(tmp), 0, size + 1);

                std::memcpy(static_cast<void*>(tmp), buf, size);

                libsigma_klog(tmp);

                delete[] tmp;
                return 0;
            };
            return node;
        }

        std::vector<std::pair<std::string, fs::fs_node*>> files;
    };
} // namespace devfs


#endif