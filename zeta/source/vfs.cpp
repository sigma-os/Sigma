#include <Zeta/vfs.h>
#include <Zeta/singleton.h>
#include <algorithm>
#include <string_view>

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

/*fs_node* vfs::get_mountpoint(uint64_t tid, std::string& path, std::string& out_local_path){
    auto absolute = this->make_path_absolute(tid, path);
    auto absolute_parts = this->split_path(absolute);

    
}*/

void* vfs::mount(fs_node* node, std::string& path){
    if(path[0] != root_char) return nullptr;

    auto parts = this->split_path(path);

    auto* current = this->mount_tree.get_root();

    void* ret = nullptr;

    // Iterate trough all parts of the path
    std::for_each(parts.begin(), parts.end(), [&](auto& part){

        bool found = false;
        // Iterate through all children of the current node
        for(auto it = current->children.begin(); it != current->children.end(); ++it){
            auto* entry = *it;
            auto& vfs_entry = entry->item;

            if(vfs_entry.name == part){
                found = true;
                current = entry;
                ret = static_cast<void*>(current);
                break;
            }
        }
        if(!found){
            // This part of the path doesn't exist, thus create a vfs_node. Not an actual file, just a reference in the VFS
            vfs_entry new_entry{};
            new_entry.name = part;
            current = this->mount_tree.insert(*current, new_entry);
            
        }
    });

    auto& vfs_entry = current->item;
    vfs_entry.file = node;
    ret = static_cast<void*>(current);

    return ret;
}

#pragma endregion

