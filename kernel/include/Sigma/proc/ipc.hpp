#ifndef SIGMA_PROC_IPC_QUEUE_H
#define SIGMA_PROC_IPC_QUEUE_H

#include <Sigma/common.h>
#include <Sigma/types/queue.h>
#include <Sigma/generic/event.hpp>

#include <klibcxx/mutex.hpp>

namespace proc::ipc
{
    class queue {
        public:
        queue(tid_t sender, tid_t receiver);
        ~queue();

        bool send(std::byte* data, size_t size);
        bool receive(std::byte* data);
        size_t get_top_message_size();

        generic::event _receive_event;

        private:
        struct message {
            size_t size;
            std::byte* data;
        };

        std::mutex _lock;
        types::queue<message> _queue;
        tid_t _sender, _receiver;
    };

    class ring {
        public:
        ring(tid_t a, tid_t b): a{a}, b{b}, _a_queue{b, a}, _b_queue{a, b} {}
        ~ring() {}

        bool send(std::byte* data, size_t size);
        bool receive(std::byte* data);
        size_t get_top_message_size();
        generic::event& get_receive_event();
        std::pair<tid_t, tid_t> get_recipients();
        

        private:
        tid_t a, b;
        queue _a_queue, _b_queue;
    };

    size_t get_message_size(uint64_t ring);
    bool send(uint64_t ring, std::byte* data, size_t size);
    bool receive(uint64_t ring, std::byte* data);
    generic::event& get_receive_event(uint64_t ring);
    std::pair<tid_t, tid_t> get_recipients(uint64_t ring);
} // namespace proc::ip


#endif