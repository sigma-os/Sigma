#ifndef SIGMA_KERNEL_TYPES_LINKED_LIST
#define SIGMA_KERNEL_TYPES_LINKED_LIST

#include <Sigma/common.h>

namespace types
{
    template<typename T>
    class linked_list {
        public:
            struct linked_list_entry {
                public:
                    template<typename... Args>
                    linked_list_entry(Args&&... args): item{T{std::forward<Args>(args)...}}, prev{}, next{} {}

                    T item;
                    linked_list_entry* prev;
                    linked_list_entry* next;
            };

            class linked_list_iterator {
                public:
                    explicit linked_list_iterator(linked_list_entry* entry): entry{entry} {}
                    T& operator*(){
                        return entry->item;
                    }
                    void operator++(){
                        if(this->entry)
                            this->entry = this->entry->next;
                    }
                    bool operator!=(linked_list_iterator it){
                        return this->entry != it.entry;
                    }
                    linked_list_entry* entry;
            };

            constexpr linked_list() noexcept : head{}, tail{}, _length{} {}

            ~linked_list() {
                if((this->head != nullptr) && (this->tail != nullptr) && (this->_length != 0)){
                    for(auto* entry = this->head; entry != nullptr; entry = entry->next)
                        delete entry;
                }
            }
            

            linked_list(const linked_list&) = delete;
            linked_list& operator=(const linked_list& other){
                this->head = nullptr;
                this->tail = nullptr;
                this->_length = 0;

                for(auto* entry = other.head; entry != nullptr; entry = entry->next)
                    this->push_back(entry->item);

                return *this;
            }

            T* push_back(T entry){
                auto* new_entry = new linked_list_entry{entry};
                new_entry->next = nullptr;
                
                if(this->_length == 0){
                    this->head = new_entry;
                    this->tail = new_entry;
                    new_entry->prev = nullptr;
                    this->_length++;
                    return &(new_entry->item);
                }

                this->tail->next = new_entry;
                new_entry->prev = this->tail;
                this->tail = new_entry;
                this->_length++;
                return &(new_entry->item);
            }


            template<typename... Args>
            T* emplace_back(Args&&... args){
                auto* new_entry = new linked_list_entry{std::forward<Args>(args)...};

                new_entry->next = nullptr;

                
                if(this->_length == 0){
                    this->head = new_entry;
                    this->tail = new_entry;
                    new_entry->prev = nullptr;
                    this->_length++;
                    return &(new_entry->item);
                }

                this->tail->next = new_entry;
                new_entry->prev = this->tail;
                this->tail = new_entry;
                this->_length++;
                return &(new_entry->item);
            }

            T* empty_entry(){
                auto* new_entry = new linked_list_entry{};
                
                new_entry->next = nullptr;

                
                if(this->_length == 0){
                    this->head = new_entry;
                    this->tail = new_entry;
                    new_entry->prev = nullptr;
                    this->_length++;
                    return &(new_entry->item);
                }

                this->tail->next = new_entry;
                new_entry->prev = this->tail;
                this->tail = new_entry;
                this->_length++;
                return &(new_entry->item);
            }

            linked_list_entry* get_entry_for_item(T* entry){
                for(auto* item = head; item != nullptr; item = item->next){
                    if(&(item->item) == entry) return item;
                }

                return nullptr;
            }

            linked_list_iterator get_iterator_for_item(T* item){
                auto* entry = get_entry_for_item(item);
                if(entry == nullptr){
                    return linked_list_iterator{nullptr};
                }
                return linked_list_iterator{entry};
            }

            linked_list_iterator begin(){
                return linked_list_iterator{head};
            }

            linked_list_iterator end(){
                return linked_list_iterator{nullptr};
            }

            NODISCARD_ATTRIBUTE
		    T& operator[](size_t index) {
			    size_t i = 0;
                for(auto& entry : *this){
                    i++;
                    if(i == index)
                        return entry;
                }

                PANIC("Index does not exist in linked_lsit");
                while(1)
                    ;
            }

            size_t length(){
                return _length;
            }


            linked_list_entry* head;
            linked_list_entry* tail;
        private:
            size_t _length;
    };
} // types

#endif