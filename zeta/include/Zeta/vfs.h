#ifndef ZETA_VFS
#define ZETA_VFS

#include <cstdint>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <Zeta/tree.h>
#include <Zeta/singleton.h>
#include <functional>

#include <libsigma/sys.h>

namespace fs {

// TODO: File things
#pragma region file_data

	enum class fs_node_types {
		file,
		directory,
		mountpoint,
		block_device,
	};

	struct fs_node; // Forward decleration for use in fs_calls

	struct fs_calls {
		//                out_node,  Path,        Mode
		std::function<int(fs_node**, const char*, int)> open;
		//                Node,     Buf,   Count,  Offset
		std::function<int(fs_node*, void*, size_t, size_t)> read;
		//                Node,     Buf,         Count,  Offset
		std::function<int(fs_node*, const void*, size_t, size_t)> write;
		//				  Node
		std::function<int(fs_node*)> close;
	};

	// TODO:
	struct fs_node {
		public:
		fs_node_types type;
		std::string path;
		uint64_t flags;
		uint64_t owner;
		uint64_t group;
		size_t length;
		fs_calls calls;
	};

#pragma endregion

#pragma region thread_data
	struct fd_data {
		bool open;
		int fd;
		fs::fs_node* node;
		int mode;

		std::uint64_t offset;
	};

	struct thread_vfs_entry {
		thread_vfs_entry(): enabled(false) {}
		void init(tid_t tid) {
			// TODO: CWD
			this->enabled = true;
			this->fd_map = std::unordered_map<int, fd_data>();
			this->tid = tid;
		}
		tid_t tid;
		int free_fd = 3; // TODO: Handle this correctly
		std::unordered_map<int, fd_data> fd_map;
		std::string cwd;
		bool enabled;
	};

#pragma endregion

#pragma region vfs_data

	struct vfs_entry {
		std::string name;
		fs_node* file;
		std::string device;
		fs_calls* fs;
	};

	class vfs {
		public:
		vfs():
			mount_list(std::vector<vfs_entry>()), thread_data(std::unordered_map<uint64_t, thread_vfs_entry>()),
			filesystems(std::unordered_map<std::string, fs_calls>()) {}

		void* mount(fs_node* node, std::string_view path, fs_calls* fs);
		void* mount(fs_node* node, std::string_view path, std::string_view fs_type);
		int open(tid_t tid, std::string_view path, int mode);
		int close(tid_t tid, int fd);
		int read(tid_t tid, int fd, void* buf, size_t count);
		int write(tid_t tid, int fd, const void* buf, size_t count);
		int seek(tid_t tid, int fd, uint64_t offset, int whence, uint64_t& new_offset);
		uint64_t tell(tid_t tid, int fd);
		int dup2(tid_t tid, int oldfd, int newfd);

		std::string make_path_absolute(tid_t tid, std::string_view path);
		std::vector<std::string_view> split_path(std::string& path);

		fs_calls* get_mountpoint(tid_t tid, std::string path, std::string& out_local_path);

		void register_fs(std::string_view fs_name, fs_calls calls);

		private:
		thread_vfs_entry& get_thread_entry(tid_t tid);
		std::vector<vfs_entry> mount_list;
		
		std::unordered_map<tid_t, thread_vfs_entry> thread_data;
		std::unordered_map<std::string, fs_calls> filesystems;
	};

#pragma endregion

#pragma region constants
	constexpr char path_separator = '/';
	constexpr char root_char = '/';
#pragma endregion

	vfs& get_vfs();

} // namespace fs


#endif