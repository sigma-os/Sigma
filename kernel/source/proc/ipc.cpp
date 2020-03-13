#include <Sigma/proc/ipc.hpp>
#include <klibc/string.h>
#include <Sigma/proc/process.h>

#include <Sigma/generic/user_handle.hpp>

proc::ipc::queue::queue(tid_t sender, tid_t receiver): _receive_event{}, _lock{}, _queue{}, _sender{sender}, _receiver{receiver} {}

proc::ipc::queue::~queue(){}

bool proc::ipc::queue::send(std::byte* data, size_t size){
    std::lock_guard guard{this->_lock};

    auto* copy = new std::byte[size];
    if(!copy)
        return false;
    memcpy(copy, data, size);

    this->_queue.push({.size = size, .data = copy});
    this->_receive_event.trigger();
    return true;
}

bool proc::ipc::queue::receive(std::byte* data){
    std::lock_guard guard{this->_lock};
    if(this->_queue.length() == 0)
        return false; // No messages on queue right now
    
    const auto msg = this->_queue.pop();
    memcpy(data, msg.data, msg.size);
    delete[] msg.data;

    return true;
}

size_t proc::ipc::queue::get_top_message_size(){
    std::lock_guard guard{this->_lock};
    if(this->_queue.length() == 0)
        return 0; // No messages on queue right now
    
    return this->_queue.back().size;
}

size_t proc::ipc::queue::get_n_messages(){
    std::lock_guard guard{this->_lock};
    return this->_queue.length();
}

bool proc::ipc::ring::send(std::byte* data, size_t size){
    tid_t tid = proc::process::get_current_tid();
    if(tid == a)
        return this->_b_queue.send(data, size);
    else if(tid == b)
        return this->_a_queue.send(data, size);
    else
        PANIC("Tried to send message on non-owned IPC ring");
}

bool proc::ipc::ring::receive(std::byte* data){
    tid_t tid = proc::process::get_current_tid();
    if(tid == a)
        return this->_a_queue.receive(data);
    else if(tid == b)
        return this->_b_queue.receive(data);
    else
        PANIC("Tried to receive message on non-owned IPC ring");
}
        
size_t proc::ipc::ring::get_top_message_size(){
    tid_t tid = proc::process::get_current_tid();
    if(tid == a)
        return this->_a_queue.get_top_message_size();
    else if(tid == b)
        return this->_b_queue.get_top_message_size();
    else{
        printf("Tried to get top message size on non-owned IPC ring: %x\n", tid);
        PANIC("");
    }
}

size_t proc::ipc::ring::get_n_messages(){
    tid_t tid = proc::process::get_current_tid();
    if(tid == a)
        return this->_a_queue.get_n_messages();
    else if(tid == b)
        return this->_b_queue.get_n_messages();
    else{
        printf("Tried to get n messages size on non-owned IPC ring: %x\n", tid);
        PANIC("");
    }
}


std::pair<tid_t, tid_t> proc::ipc::ring::get_recipients(){
    return {this->a, this->b};
}

generic::event& proc::ipc::ring::get_receive_event(){
    tid_t tid = proc::process::get_current_tid();
    if(tid == a)
        return this->_a_queue._receive_event;
    else if(tid == b)
        return this->_b_queue._receive_event;
    else
        PANIC("Tried to get event size on non-owned IPC ring");
}

static proc::ipc::ring* get_ring(uint64_t ring){
    auto* thread = proc::process::get_current_thread();
    ASSERT(thread);

    auto* handle = thread->handle_catalogue.get<generic::handles::ipc_ring_handle>(ring);
    ASSERT(handle);

    auto* ipc_ring = handle->ring;
    ASSERT(ipc_ring);

    return ipc_ring;
}

size_t proc::ipc::get_message_size(uint64_t ring){
    return get_ring(ring)->get_top_message_size();
}

size_t proc::ipc::get_n_messages(uint64_t ring){
    return get_ring(ring)->get_n_messages();
}

bool proc::ipc::send(uint64_t ring, std::byte* data, size_t size){
    return get_ring(ring)->send(data, size);
}

bool proc::ipc::receive(uint64_t ring, std::byte* data){
    return get_ring(ring)->receive(data);
}

generic::event& proc::ipc::get_receive_event(uint64_t ring){
    return get_ring(ring)->get_receive_event();
}

std::pair<tid_t, tid_t> proc::ipc::get_recipients(uint64_t ring){
    return get_ring(ring)->get_recipients();
}