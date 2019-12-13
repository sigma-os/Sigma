#include <Zeta/vfs.h>
#include <Zeta/singleton.h>
#include <algorithm>
#include <string_view>
#include <cstring>
#include <fcntl.h>

using namespace fs;


#pragma region global_vfs

vfs global_vfs;

vfs& fs::get_vfs(){
    return global_vfs;
}

#pragma endregion



#pragma region vfs

thread_vfs_entry& vfs::get_thread_entry(tid_t tid){
    auto& data = this->thread_data[tid];
    if(!data.enabled) data.init(tid);
    return data;
}

std::string vfs::make_path_absolute(tid_t tid, std::string_view path){
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

fs_calls* vfs::get_mountpoint(tid_t tid, std::string path, std::string& out_local_path) {
	auto absolute = this->make_path_absolute(tid, path);

    size_t guess_size = 0;
    vfs_entry* guess = nullptr;

    std::for_each(this->mount_list.begin(), this->mount_list.end(), [&](auto& mountpoint){
        size_t mount_len = mountpoint.name.length();
        if(!(absolute.length() < mount_len)){
			if(std::memcmp(absolute.c_str(), mountpoint.name.c_str(), mount_len) == 0) {
				if((absolute[mount_len] == path_separator || absolute[mount_len] == '\0' ||
					std::strcmp(mountpoint.name.c_str(), "/") == 0) &&
			    	mount_len > guess_size) {
				
					guess_size = mount_len;
					guess = &mountpoint;
				}
			}
		}		
	});

    out_local_path = path;
    if(guess_size > 1) out_local_path = out_local_path.substr(guess_size);
    if(out_local_path[0] == '\0') out_local_path[0] = '/';
	
	if(!guess) return nullptr;
	else return guess->fs;
}

void* vfs::mount(fs_node* node, std::string_view path, fs_calls* fs) {
	if(path[0] != root_char)
		return nullptr;

	auto& entry = this->mount_list.emplace_back();
	entry.file = node;
	entry.name = path;
	entry.fs = fs;

    return nullptr;
}

void* vfs::mount(fs_node* node, std::string_view path, std::string_view fs_type) {
	if(path[0] != root_char)
		return nullptr;

	#if 0 // This works for C++20 but not C++17
	if(!this->filesystems.contains(fs_type))
		return nullptr;
	#else // C++17
	if(this->filesystems.find(std::string{fs_type}) == this->filesystems.end())
		return nullptr;
	#endif

	auto& entry = this->mount_list.emplace_back();
	entry.file = node;
	entry.name = path;
	entry.fs = &this->filesystems[std::string{fs_type}];

	return nullptr;
}


int vfs::open(tid_t tid, std::string_view path, int mode) {
	std::string out_local_path{};
	auto* fs_calls_node = this->get_mountpoint(tid, std::string{path}, out_local_path);
	if(fs_calls_node == nullptr)
		return -1;

	fs_node* real_node = nullptr;
	int res = fs_calls_node->open(&real_node, out_local_path.c_str(), mode);
	if(res == -1)
		return -1;

	auto& thread = this->get_thread_entry(tid);
    
    int fd = thread.free_fd;
    thread.free_fd++;

	thread.fd_map[fd] = {.open = true, .fd = fd, .node = real_node, .mode = mode, .offset = 0};

    return fd;
}

int vfs::read(tid_t tid, int fd, void* buf, size_t count){
	auto& thread = this->get_thread_entry(tid);
	auto& file_descriptor = thread.fd_map[fd];
	if(file_descriptor.open){ // Check for initialization
		//TODO: Check for permission

		return file_descriptor.node->calls.read(file_descriptor.node, buf, count, file_descriptor.offset);
	}

	return -1;
}

int vfs::write(tid_t tid, int fd, const void* buf, size_t count){
	auto& thread = this->get_thread_entry(tid);
	auto& file_descriptor = thread.fd_map[fd];
	if(file_descriptor.open){ // Check for initialization
		//TODO: Check for permission

		return file_descriptor.node->calls.write(file_descriptor.node, buf, count, file_descriptor.offset);
	}

	return -1;
}

int vfs::seek(tid_t tid, int fd, uint64_t offset, int whence, uint64_t& ret){
	auto& thread = this->thread_data[tid];
	auto& file_descriptor = thread.fd_map[fd];
	if(file_descriptor.open && file_descriptor.node->type == fs_node_types::file){ // Check for initialization
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

	// TODO: Something something ESPIPE
	return 0;
}

uint64_t vfs::tell(tid_t tid, int fd){
	auto& thread = this->thread_data[tid];
	auto& file_descriptor = thread.fd_map[fd];
	if(file_descriptor.open){ // Check for initialization
		//TODO: Check for permission
		//TODO: Pipes
		return file_descriptor.offset;
	}
	return -1;
}

int vfs::dup2(tid_t tid, int oldfd, int newfd){
	auto& thread_entry = this->get_thread_entry(tid);

	if(!thread_entry.fd_map[oldfd].open) return -1;
	if(oldfd == newfd && thread_entry.fd_map[oldfd].open) return newfd;

	if(thread_entry.fd_map[newfd].open) // If already open, close
		vfs::close(tid, newfd);
	
	thread_entry.fd_map[newfd] = thread_entry.fd_map[oldfd];
	thread_entry.fd_map[newfd].fd = newfd; // Copy everything over, except this
	
	return newfd;
}

int vfs::close(tid_t tid, int fd){
	auto& thread_entry = this->get_thread_entry(tid);
	auto& fd_entry = thread_entry.fd_map[fd];
	if(fd_entry.open){
		if(fd_entry.node->calls.close)
			fd_entry.node->calls.close(fd_entry.node);
		fd_entry.open = false;
	}
	return 0;
}

void vfs::register_fs(std::string_view fs_name, fs_calls calls){
	this->filesystems[std::string{fs_name}] = calls;
}



#pragma endregion

