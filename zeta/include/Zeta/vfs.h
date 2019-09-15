#ifndef ZETA_VFS
#define ZETA_VFS

#include <cstdint>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <Zeta/tree.h>
#include <Zeta/singleton.h>

namespace fs
{
    // TODO: File things
    #pragma region file_data

    // TODO:
    struct fs_node {
        public:
        std::string path;
        uint64_t flags;
        uint64_t owner;
        uint64_t group;
    };

    #pragma endregion

    #pragma region thread_data
    struct fd_data {
        fd_data(): fd(0), offset(0) {}
        int fd;
        std::uint64_t offset;
    };

    struct thread_vfs_entry {
        thread_vfs_entry(): enabled(false) {}
        void init(uint64_t tid){
            // TODO: CWD
            this->enabled = true;
            this->fd_map = std::unordered_map<int, fd_data>();
            this->tid = tid;
        }
        // TODO: use tid_t or pid_t
        uint64_t tid;
        std::unordered_map<int, fd_data> fd_map;
        std::string cwd;
        bool enabled;
    };

    #pragma endregion

    #pragma region vfs_data

    class vfs {
        public:
        vfs(): mount_tree(tree<fs_node>()), thread_data(std::unordered_map<uint64_t, thread_vfs_entry>()) {}

        void mount(uint64_t tid, fs_node* node, std::string_view path);

        std::string make_path_absolute(uint64_t tid, std::string_view path);
        std::vector<std::string_view> split_path(std::string& path);

        private:
        thread_vfs_entry& get_thread_entry(uint64_t tid);
        tree<fs_node> mount_tree;
        // TODO: use tid_t
        std::unordered_map<uint64_t, thread_vfs_entry> thread_data;
    };

    #pragma endregion

    #pragma region constants
    constexpr char path_separator = '/';
    constexpr char root_char = '/';
    #pragma endregion

    vfs& get_vfs();

} // namespace vfs


#endif