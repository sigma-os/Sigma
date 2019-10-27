#ifndef SIGMA_KERNEL_TYPES_LINKED_LIST
#define SIGMA_KERNEL_TYPES_LINKED_LIST

#include <Sigma/common.h>
#include <klibcxx/mutex.hpp>
#include <Sigma/arch/x86_64/misc/spinlock.h>

namespace types
{
    template<typename T>
    class linked_list {
        public:
            template<typename entry_T>
            struct linked_list_entry {
                public:
                    linked_list_entry(): item(entry_T()), prev(nullptr), next(nullptr) {}
                    entry_T item;
                    linked_list_entry<entry_T>* prev;
                    linked_list_entry<entry_T>* next;
            };

            template<typename iterator_T>
            class linked_list_iterator {
                public:
                    explicit linked_list_iterator(linked_list_entry<iterator_T>* entry): entry(entry) { }
                    T& operator*(){
                        return entry->item;
                    }
                    void operator++(){
                        if(this->entry) this->entry = this->entry->next;
                    }
                    bool operator!=(linked_list_iterator it){
                        if(this->entry == it.entry) return false;
                        return true;
                    }
                    linked_list_entry<iterator_T>* entry;
            };

            linked_list(): head(nullptr), tail(nullptr), mutex(x86_64::spinlock::mutex()), length(0) {}

            ~linked_list() {
                if((this->head != nullptr) && (this->tail != nullptr) && (this->length != 0)){
                    for(T& e : *this){
                        delete &e;
                    }
                }
            }

            T* push_back(T entry){
                std::lock_guard guard{this->mutex};
                linked_list_entry<T>* new_entry = new linked_list_entry<T>;

                new_entry->item = entry;
                new_entry->next = nullptr;

                
                if(this->length == 0){
                    this->head = new_entry;
                    this->tail = new_entry;
                    new_entry->prev = nullptr;
                    this->length++;
                    return &(new_entry->item);
                }

                this->tail->next = new_entry;
                new_entry->prev = this->tail;
                this->tail = new_entry;
                this->length++;
                return &(new_entry->item);
            }

            T* empty_entry(){
                std::lock_guard guard{this->mutex};
                linked_list_entry<T>* new_entry = new linked_list_entry<T>;
                
                new_entry->next = nullptr;

                
                if(this->length == 0){
                    this->head = new_entry;
                    this->tail = new_entry;
                    new_entry->prev = nullptr;
                    this->length++;
                    return &(new_entry->item);
                }

                this->tail->next = new_entry;
                new_entry->prev = this->tail;
                this->tail = new_entry;
                this->length++;
                return &(new_entry->item);
            }

            linked_list_entry<T>* get_entry_for_item(T* entry){
                for(linked_list_entry<T>* item = head; item != nullptr; item = item->next){
                    if(&(item->item) == entry) return item;
                }

                return nullptr;
            }

            linked_list_iterator<T> get_iterator_for_item(T* item){
                linked_list_entry<T>* entry = get_entry_for_item(item);
                if(entry == nullptr){
                    return linked_list_iterator<T>(nullptr);
                }
                return linked_list_iterator<T>(entry);
            }

            linked_list_iterator<T> begin(){
                return linked_list_iterator<T>(head);
            }

            linked_list_iterator<T> end(){
                return linked_list_iterator<T>(nullptr);
            }


            linked_list_entry<T>* head;
            linked_list_entry<T>* tail;
        private:
            x86_64::spinlock::mutex mutex;
            size_t length;
    };
} // types

#endif