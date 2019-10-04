#include <Zeta/vfs.h>
#include <Zeta/singleton.h>
#include <algorithm>
#include <string_view>
#include <cstring>
#include <fcntl.h>

using namespace fs;


#pragma region global_vfs

vfs global_vfs;

vfs& get_vfs(){
    return global_vfs;
}

#pragma endregion



#pragma region vfs

thread_vfs_entry& vfs::get_thread_entry(uint64_t tid){
    auto& data = this->thread_data[tid];
    if(!data.enabled) data.init(tid);
    return data;
}

std::string vfs::make_path_absolute(uint64_t tid, std::string_view path){
    auto& cwd = this->get_thread_entry(tid).cwd;
    auto absolute = std::string();
    auto ret = std::string();

    if(path[0] != root_char){
        absolute.append(cwd);
		if(cwd[cwd.size() - 1] != root_char) absolute.push_back(root_char);

	}
    absolute.append(path);

    // Resolve ret
    auto parts = this->split_path(absolute);
    for(auto it = parts.begin(); it != parts.end(); ++it){
        auto& item = *it;

        if(item == "."){
            parts.erase(it); // "." refers to the current directory and thus has no effect on the dir
        } else if(item == ".."){
            parts.erase(it); // ".." refers to the parent dir so we should go one up and erase the previous one
            if(it != parts.begin())
                parts.erase(it - 1); // `parts.begin() - 1` is UB so check for that
            it -= 1; // Move back iterator by 1 since we erased 1 and otherwise would skip the next item
        }
    }

    std::for_each(parts.begin(), parts.end(), [&](auto& item){
        ret.push_back('/');
        ret.append(item);
    });

    return ret;
}

std::vector<std::string_view> vfs::split_path(std::string& path){
    auto ret = std::vector<std::string_view>();
    for(uint64_t i = 0; i < path.length(); i++){
        char c = path[i];
        if(c == path_separator){
            uint64_t j;
            for(j = i + 1; j < path.length(); j++)
                if(path[j] == path_separator)
                    break;
            
            size_t size = j - i;
            if(size != 1)
                ret.push_back(std::string_view(path).substr(i + 1, size - 1));
        }
    }

    return ret;
}

fs_calls* vfs::get_mountpoint(uint64_t tid, std::string& path, std::string& out_local_path) {
	auto absolute = this->make_path_absolute(tid, path);

    size_t guess_size = 0;
    vfs_entry* guess = nullptr;

    std::for_each(this->mount_list.begin(), this->mount_list.end(), [&](auto& mountpoint){
        size_t mount_len = mountpoint.name.length();
        if(absolute.length() < mount_len) return;

		if(std::memcmp(absolute.c_str(), mountpoint.name.c_str(), mount_len) == 0) {
			if((absolute[mount_len] == path_separator || absolute[mount_len] == '\0' ||
				std::strcmp(mountpoint.name.c_str(), "/") == 0) &&
			    mount_len > guess_size) {
				
				guess_size = mount_len;
				guess = &mountpoint;
			}
		}
	});

    out_local_path = path;

    if(guess_size > 1) out_local_path = out_local_path.substr(guess_size);

    if(out_local_path[0] == '\0') out_local_path[0] = '/';

	return guess->fs;
}

void* vfs::mount(fs_node* node, std::string& path, fs_calls* fs) {
	if(path[0] != root_char)
		return nullptr;

	auto& entry = this->mount_list.emplace_back();
	entry.file = node;
	entry.name = path;
	entry.fs = fs;

    return nullptr;
}

void* vfs::mount(fs_node* node, std::string& path, std::string& fs_type) {
	if(path[0] != root_char)
		return nullptr;

	#if 0 // This works for C++20 but not C++17
	if(!this->filesystems.contains(fs_type))
		return nullptr;
	#else // C++17
	if(this->filesystems.find(fs_type) == this->filesystems.end())
		return nullptr;
	#endif

	auto& entry = this->mount_list.emplace_back();
	entry.file = node;
	entry.name = path;
	entry.fs = &this->filesystems[fs_type];

	return nullptr;
}


int vfs::open(uint64_t tid, std::string& path, int mode) {
	std::string out_local_path{};
	auto* fs_calls_node = this->get_mountpoint(tid, path, out_local_path);
	if(fs_calls_node == nullptr)
		return -1;

	fs_node* real_node = nullptr;
	int res = fs_calls_node->open(&real_node, out_local_path.c_str(), mode);
	if(res == -1)
		return -1;

	auto& thread = this->get_thread_entry(tid);
    
    int fd = thread.free_fd;
    thread.free_fd++;

	thread.fd_map[fd] = {.fd = fd, .node = real_node, .mode = mode, .offset = 0};

    return fd;
}

int vfs::read(uint64_t tid, int fd, void* buf, size_t count){
	auto& thread = this->get_thread_entry(tid);
	auto& file_descriptor = thread.fd_map[fd];
	if(file_descriptor.fd == fd){ // Check for initialization
		//TODO: Check for permission

		return file_descriptor.node->calls.read(file_descriptor.node, buf, count, file_descriptor.offset);
	}

	return -1;
}

int vfs::write(uint64_t tid, int fd, const void* buf, size_t count){
	auto& thread = this->get_thread_entry(tid);
	auto& file_descriptor = thread.fd_map[fd];
	if(file_descriptor.fd == fd){ // Check for initialization
		//TODO: Check for permission

		return file_descriptor.node->calls.write(file_descriptor.node, buf, count, file_descriptor.offset);
	}

	return -1;
}

int vfs::seek(uint64_t tid, int fd, uint64_t offset, int whence, uint64_t& ret){
	auto& thread = this->thread_data[tid];
	auto& file_descriptor = thread.fd_map[fd];
	if(file_descriptor.fd == fd){ // Check for initialization
		//TODO: Check for permission
		//TODO: Pipes
		switch (whence)
		{
		case SEEK_SET:
			file_descriptor.offset = offset;
			ret = offset;
			break;

		case SEEK_CUR:
			ret = file_descriptor.offset;
			break;

		case SEEK_END:
			file_descriptor.offset = file_descriptor.node->length;
			ret = file_descriptor.offset;
			break;
		
		default:
			break;
		}
	}

	return -1;
}

void vfs::register_fs(std::string& fs_name, fs_calls calls){
	this->filesystems[fs_name] = calls;
}



#pragma endregion

