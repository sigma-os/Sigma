#ifndef SIGMA_KERNEL_TYPES_LINKED_LIST
#define SIGMA_KERNEL_TYPES_LINKED_LIST

#include <Sigma/common.h>

namespace types
{
    template<typename T>
    struct linked_list_entry {
        public:
        linked_list_entry(): item(T()), prev(nullptr), next(nullptr) {}
        T item;
        linked_list_entry<T>* prev;
        linked_list_entry<T>* next;
    };

    template<typename T>
    class linked_list_iterator {
        public:
        explicit linked_list_iterator(linked_list_entry<T>* entry): entry(entry) { }
        T& operator*(){
            return entry->item;
        }
        void operator++(){
            if(this->entry) this->entry = this->entry->next;
        }
        bool operator!=(linked_list_iterator& it){
            //if((this->entry->item == it.entry->item) && (this->entry->prev == it.entry->prev) && (this->entry->next == it.entry->next)) return false;
            if(this->entry == it.entry) return false;

            return true;
        }
        linked_list_entry<T>* entry;
    };

    template<typename T>
    class linked_list {
        public:
            linked_list(): head(nullptr), tail(nullptr), length(0) {};
            ~linked_list() {
                for(T& e : *this){
                    delete &e;
                }
            }

            void push_back(T entry){
                linked_list_entry<T>* new_entry = new linked_list_entry<T>;

                new_entry->item = entry;
                new_entry->next = nullptr;

                
                if(this->length == 0){
                    this->head = new_entry;
                    this->tail = new_entry;
                    new_entry->prev = nullptr;
                    this->length++;
                    return;
                }

                this->tail->next = new_entry;
                new_entry->prev = this->tail;
                this->tail = new_entry;
                this->length++;
            }

            linked_list_iterator<T> begin(){
                return linked_list_iterator<T>(head);
            }

            linked_list_iterator<T> end(){
                return linked_list_iterator<T>(nullptr);
            }


            linked_list_entry<T>* head;
            linked_list_entry<T>* tail;
            size_t length;
        private:
    };
} // types

#endif