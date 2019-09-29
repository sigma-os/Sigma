#ifndef ZETA_VFS
#define ZETA_VFS

#include <cstdint>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <Zeta/tree.h>
#include <Zeta/singleton.h>
#include <functional>

namespace fs {
	template<typename Ret, typename... Args>
	class fs_call {
		// TODO: Add IPC call
		public:
		fs_call(std::function<Ret(Args...)> func): func(func), type(call_types::function) {}

		using func_ptr_t = Ret (*)(Args...);
		fs_call(func_ptr_t func_ptr): func_ptr(func_ptr), type(call_types::function_ptr) {}


		Ret operator()(Args... args) {
			switch(this->type) {
				case call_types::function:
					return this->func(args...);

				case call_types::function_ptr:
					return this->func_ptr(args...);

				default:
					std::cerr << "Unknown call type";
					break;
			}
			return {};
		}

		private:
		enum class call_types { function, function_ptr };
		call_types type;

		std::function<Ret(Args...)> func;
		func_ptr_t func_ptr;
	};

// TODO: File things
#pragma region file_data

	enum class fs_node_types {
		file,
		directory,
		mountpoint,
		block_device,
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
		fs_call<int, const char*, int> open;
		//           Node,     Buf,   Count,  Offset
		fs_call<int, fs_node*, void*, size_t, size_t> read;
		//           Node,     Buf,         Count,  Offset
		fs_call<int, fs_node*, const void*, size_t, size_t> write;
	};

#pragma endregion

#pragma region thread_data
	struct fd_data {
		int fd;
		fs::fs_node* node;
		int mode;

		std::uint64_t offset;
	};

	struct thread_vfs_entry {
		thread_vfs_entry(): enabled(false) {}
		void init(uint64_t tid) {
			// TODO: CWD
			this->enabled = true;
			this->fd_map = std::unordered_map<int, fd_data>();
			this->tid = tid;
		}
		// TODO: use tid_t or pid_t
		uint64_t tid;
		int free_fd = 500; // TODO: Handle this correctly
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
		std::string fs_type;
	};

	class vfs {
		public:
		vfs():
			mount_list(std::vector<vfs_entry>()), thread_data(std::unordered_map<uint64_t, thread_vfs_entry>()) {}

		void* mount(fs_node* node, std::string& path);
		int open(uint64_t tid, std::string& path, int mode);
		int read(uint64_t tid, int fd, void* buf, size_t count);
		int write(uint64_t tid, int fd, const void* buf, size_t count);
		int seek(uint64_t tid, int fd, uint64_t offset, int whence, uint64_t& new_offset);

		std::string make_path_absolute(uint64_t tid, std::string_view path);
		std::vector<std::string_view> split_path(std::string& path);

		fs_node* get_mountpoint(uint64_t tid, std::string& path, std::string& out_local_path);

		private:
		thread_vfs_entry& get_thread_entry(uint64_t tid);
		std::vector<vfs_entry> mount_list;
		// TODO: use tid_t
		std::unordered_map<uint64_t, thread_vfs_entry> thread_data;
	};

#pragma endregion

#pragma region constants
	constexpr char path_separator = '/';
	constexpr char root_char = '/';
#pragma endregion

	vfs& get_vfs();

} // namespace fs


#endif