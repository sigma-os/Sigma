#ifndef SIGMA_KERNEL_PROC_IPC
#define SIGMA_KERNEL_PROC_IPC

#include <Sigma/common.h>
#include <klibc/stdio.h>
#include <klibc/string.h>

#include <Sigma/types/vector.h>

namespace proc::ipc {
	constexpr uint16_t ipc_message_header_magic_low = 0xBEEF;
	constexpr uint16_t ipc_message_header_magic_high = 0xC0DE;

	struct ipc_message_footer; // Forward declaration so we can have a pointer to it in ipc_message_header

	struct ipc_message_header {
		public:
		ipc_message_header(tid_t target, tid_t origin, size_t buffer_length): target(target), origin(origin), buffer_length(buffer_length){
			this->set_magic();
		}

		~ipc_message_header(){
			if(!this->check_magic()){
				printf("[IPC]: Tried to destruct ipc_message_header with invalid magic");
			}
		}

		[[nodiscard]]
		ALWAYSINLINE_ATTRIBUTE
		bool check_magic() {
			if (this->magic_low != ipc_message_header_magic_low || this->magic_high != ipc_message_header_magic_high) return false;
			else return true;
		}

		ALWAYSINLINE_ATTRIBUTE
		void set_magic() {
			this->magic_low = ipc_message_header_magic_low;
			this->magic_high = ipc_message_header_magic_high;
		}
		
		uint16_t magic_low;
		tid_t target;
		tid_t origin;
		size_t buffer_length;
		ipc_message_footer* footer;
		uint16_t magic_high;
	};

	constexpr uint16_t ipc_message_footer_magic_low = 0xBAAD;
	constexpr uint16_t ipc_message_footer_magic_high = 0x1EE7;

	struct ipc_message_footer {
		public:
		ipc_message_footer(){
			this->set_magic();
		}

		~ipc_message_footer(){
			if(!this->check_magic()){
				printf("[IPC]: Tried to destruct ipc_message_footer with invalid magic");
			}
		}

		[[nodiscard]]
		ALWAYSINLINE_ATTRIBUTE
		bool check_magic(){
			if(this->magic_low != ipc_message_footer_magic_low || this->magic_high != ipc_message_footer_magic_high) return false;
			else return true;
		}

		ALWAYSINLINE_ATTRIBUTE
		void set_magic(){
			this->magic_low = ipc_message_footer_magic_low;
			this->magic_high = ipc_message_footer_magic_high;
		}

		uint16_t magic_low;
		ipc_message_header* header;
		uint16_t magic_high;
	};

	constexpr size_t thread_ipc_manager_default_msg_buffer_size = 128;

	class thread_ipc_manager
	{
	public:
		thread_ipc_manager(tid_t tid);
		thread_ipc_manager(){}
		~thread_ipc_manager();

		// Async
		bool send_message(tid_t origin, size_t buffer_length, uint8_t* buffer);
		bool receive_message(tid_t& origin, size_t& size, types::vector<uint8_t>& data);

		// Sync
		void receive_message_sync(tid_t& origin, size_t& size, types::vector<uint8_t>& data);
	private:
		uint8_t* msg_buffer;
		uint64_t current_offset;
		uint64_t current_unread_messages_count;
		tid_t tid;
	};
}

#endif