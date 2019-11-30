#include <Sigma/proc/ipc.h>
#include <Sigma/proc/process.h>

using namespace proc::ipc;

void thread_ipc_manager::init(tid_t tid)
{
	std::lock_guard guard{this->lock};
	this->current_offset = 0;
	this->current_buffer_size = thread_ipc_manager_default_msg_buffer_size;
	this->current_unread_messages_count = 0;
	this->tid = tid;
	this->msg_buffer = reinterpret_cast<uint8_t*>(malloc(this->current_buffer_size));
	if (this->msg_buffer == nullptr) {
		printf("[IPC]: Couldn't allocate buffer for ipc manager for tid: %d\n", tid);
		return;
	}
	memset(reinterpret_cast<void*>(this->msg_buffer), 0, this->current_buffer_size);
}

void thread_ipc_manager::deinit()
{
	std::lock_guard guard{this->lock};
	if (this->current_offset >= this->current_buffer_size) {
		printf("[IPC]: Detected buffer overrun while destructing thread_ipc_manager [%x], leaking memory..\n", this);
		return;
	}
	if (this->msg_buffer) free(reinterpret_cast<void*>(this->msg_buffer));
}

/* The on memory layout of the message looks like this
 * | ipc_message_header
 * | uint8_t[] data
 * | ipc_message_footer
 */
bool thread_ipc_manager::send_message(tid_t origin, size_t buffer_length, uint8_t* buffer) {
	std::lock_guard guard{this->lock};
	if (this->current_offset + (sizeof(ipc::ipc_message_header) + buffer_length) >= thread_ipc_manager_default_msg_buffer_size) {
		
		size_t size = this->current_buffer_size * 2;
		this->msg_buffer = static_cast<uint8_t*>(realloc(static_cast<void*>(this->msg_buffer), size));
		this->current_buffer_size = size;
	}

	ipc_message_header* header_ptr = reinterpret_cast<ipc_message_header*>(this->msg_buffer + this->current_offset);
	new (header_ptr) ipc_message_header(this->tid, origin, buffer_length);
	this->current_offset += sizeof(ipc_message_header);

	memcpy(reinterpret_cast<void*>(this->msg_buffer + this->current_offset), reinterpret_cast<void*>(buffer), buffer_length);
	this->current_offset += buffer_length;

	ipc_message_footer* footer_ptr = reinterpret_cast<ipc_message_footer*>(this->msg_buffer + this->current_offset);
	new (footer_ptr) ipc_message_footer();
	this->current_offset += sizeof(ipc_message_footer);

	header_ptr->footer = footer_ptr;
	footer_ptr->header = header_ptr;

	this->current_unread_messages_count++;
	
	proc::process::wake_if_blocked(tid, proc::process::block_reason::WAITING_FOR_IPC);

	return true;
}

bool thread_ipc_manager::receive_message(tid_t* origin, size_t* size, uint8_t* data)
{
	std::lock_guard guard{this->lock};
	if (this->current_unread_messages_count == 0) return false; // No new messages

	ipc_message_footer* footer = reinterpret_cast<ipc_message_footer*>(this->msg_buffer + this->current_offset - sizeof(ipc_message_footer));
	if(!footer->check_magic()){
		printf("[IPC]: Footer [%x] magic invalid", footer);
		return false;
	}

	ipc_message_header* header = footer->header;
	if(!header->check_magic()){
		printf("[IPC]: Header [%x] magic invalid", header);
		return false;
	}


	*origin = header->origin;
	*size = header->buffer_length;

	uint8_t* raw_msg = reinterpret_cast<uint8_t*>(header + 1);
	memcpy(static_cast<void*>(data), static_cast<void*>(raw_msg), header->buffer_length);

	this->current_unread_messages_count--;
	this->current_offset -= (sizeof(ipc_message_header) + header->buffer_length + sizeof(ipc_message_footer));
	return true;
}

size_t thread_ipc_manager::get_msg_size(){
	if(this->current_unread_messages_count == 0)
		return 0;

	std::lock_guard guard{this->lock};

	ipc_message_footer* footer = reinterpret_cast<ipc_message_footer*>(this->msg_buffer + this->current_offset - sizeof(ipc_message_footer));
	if(!footer->check_magic()){
		printf("[IPC]: Footer [%x] magic invalid", footer);
		return 0;
	}

	ipc_message_header* header = footer->header;
	if(!header->check_magic()){
		printf("[IPC]: Header [%x] magic invalid", header);
		return 0;
	}

	return header->buffer_length;
}

void thread_ipc_manager::receive_message_sync(tid_t& origin, size_t& size, uint8_t* data){
	if(this->current_unread_messages_count != 0){
		this->receive_message(&origin, &size, data);
		return;
	}

	volatile uint64_t old_count = this->current_unread_messages_count;
	while(this->current_unread_messages_count == old_count);
	this->receive_message(&origin, &size, data);
}