#ifndef SIGMA_TYPES_HASH_MAP_H
#define SIGMA_TYPES_HASH_MAP_H

#include <Sigma/common.h>
#include <Sigma/types/linked_list.h>
#include <klibcxx/utility.hpp>

namespace types
{   
    // TODO: Seriously, this is the best hash_map you can think of, just, make something doable, not this monstrosity
    template<typename Key, typename Value, typename Hash>
    class hash_map {
        public:
        hash_map(): hasher{} {}

        void push_back(Key key, Value value){
            auto hash = this->hasher(key);

            this->list.push_back({hash, value});
        }

        Value& operator[](Key key){
            auto hash = this->hasher(key);

            for(auto& entry : list)
                if(entry.first == hash)
                    return entry.second;

            PANIC("Hash not in map");
            while(1)
                ;
        }

        private:
        using entry = std::pair<typename Hash::hash_result, Value>;

        types::linked_list<entry> list;

        Hash hasher;
    };
} // namespace types


#endif