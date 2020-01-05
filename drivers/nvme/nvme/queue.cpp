#include "queue.hpp"
#include <sys/mman.h>
#include <iostream>
#include <string.h>
#include <libdriver/bit.hpp>

using namespace nvme;

queue_pair::submission_info::submission_info(size_t n_entries, uint16_t* doorbell): doorbell{doorbell} {
    size_t queue_size = sizeof(regs::command) * n_entries;

    if(libsigma_get_phys_region(queue_size, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
        std::cerr << "nvme: Couldn't allocate submission queue\n";
        return;
    }

    this->queue = (regs::command*)region.virtual_addr;

    this->head = 0;
    this->tail = 0;
}

queue_pair::completion_info::completion_info(size_t n_entries, uint16_t* doorbell): doorbell{doorbell} {
    size_t queue_size = sizeof(regs::completion) * n_entries;

    if(libsigma_get_phys_region(queue_size, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
        std::cerr << "nvme: Couldn't allocate completion queue\n";
        return;
    }

    this->queue = (regs::completion*)region.virtual_addr;

    this->head = 0;
    this->tail = 0;
    this->expected_phase = 1;     
}

queue_pair::queue_pair(size_t n_entries, uint16_t* submission_doorbell, uint16_t* completion_doorbell, qid_t qid): n_entries{n_entries}, qid{qid}, available_cids{bitmap<n_commands>{}}, submission{submission_info{n_entries, submission_doorbell}}, completion{completion_info{n_entries, completion_doorbell}}{}

cid_t queue_pair::send_command(regs::command* cmd){
    cid_t cid = this->available_cids.get_free_bit();

    cmd->header.cid = cid;

    uint16_t tail = submission.tail;
    memcpy((void*)(submission.queue + tail), cmd, 64);
    tail++;

    if(tail == n_entries)
        tail = 0; // Wrap around if end is reached

    *submission.doorbell = tail;
    submission.tail = tail;

    return cid;
}

bool queue_pair::send_and_wait(regs::command* cmd){
    uint16_t head = completion.head;

    auto cid = send_command(cmd);

    while(true){
        bool phase = completion.queue[completion.head].status.phase;
        if(phase == completion.expected_phase)
            break;
    }

    uint8_t status_code = completion.queue[completion.head].status.code;
    if(status_code){
        std::cerr << "nvme: Error in completion queue, code: 0x" << std::hex << status_code << std::endl;
        return false;
    }

    head++;
    if(head == n_entries){
        head = 0;
        completion.expected_phase = !completion.expected_phase;
    }

    *completion.doorbell = head;
    completion.head = head;

    return true;
}

size_t queue_pair::get_n_entries(){
    return this->n_entries;
}

uintptr_t queue_pair::get_submission_phys_base(){
    return this->submission.region.physical_addr;
}

uintptr_t queue_pair::get_completion_phys_base(){
    return this->completion.region.physical_addr;
}