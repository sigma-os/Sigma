#ifndef SIGMA_TYPES_HASH_MAP_H
#define SIGMA_TYPES_HASH_MAP_H

#include <Sigma/common.h>
#include <Sigma/types/linked_list.h>
#include <klibcxx/utility.hpp>
#include <Sigma/arch/x86_64/misc/spinlock.h>

namespace types
{   
    template<typename T>
    struct nop_hasher {
        using hash_result = T;

        hash_result operator()(T item){
            return item;
        }
    };

    // TODO: Seriously, this is the best hash_map you can think of, just, make something doable, not this monstrosity
    template<typename Key, typename Value, typename Hasher>
    class hash_map {
        public:
        hash_map() = default;
        hash_map(hash_map&& other){
            this->list = std::move(other.list);
            this->hasher = std::move(other.hasher);
        }

        hash_map& operator=(hash_map&& other){
            this->list = std::move(other.list);
            this->hasher = std::move(other.hasher);

            return *this;
        }

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
        using entry = std::pair<typename Hasher::hash_result, Value>;

        types::linked_list<entry> list;

        Hasher hasher;
    };
} // namespace types


#endif