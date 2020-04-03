#ifndef SIGMA_GENERIC_USER_HANDLE_H
#define SIGMA_GENERIC_USER_HANDLE_H

#include <Sigma/common.h>
#include <Sigma/types/hash_map.hpp>
#include <Sigma/generic/virt.hpp>
#include <Sigma/proc/ipc.hpp>

#include <klibcxx/utility.hpp>

#include <Sigma/generic/event.hpp>

namespace generic::handles
{
	enum class handle_type {vCpu, vSpace, irq, ipcRing};

	struct handle {
		handle(handle_type type): type{type} {}
		handle_type type;

		virtual ~handle() {}
	};

	struct vcpu_handle : public handle {
		vcpu_handle(generic::virt::vspace* space): handle{handle_type::vCpu}, cpu{space} {}
		
		static constexpr handle_type default_type = handle_type::vCpu;

		generic::virt::vcpu cpu;
	};

	struct vspace_handle : public handle {
		vspace_handle(): handle{handle_type::vSpace}, space{} {}
	
		static constexpr handle_type default_type = handle_type::vSpace;

		generic::virt::vspace space;
	};

	struct irq_handle : public handle {
		irq_handle(uint8_t vector): handle{handle_type::irq}, vector{vector}, event{} {}

		static constexpr handle_type default_type = handle_type::irq;

		uint8_t vector;
		generic::event event;
	};

	struct ipc_ring_handle : public handle {
		ipc_ring_handle(proc::ipc::ring* ring): handle{handle_type::ipcRing}, ring{ring} {}

		static constexpr handle_type default_type = handle_type::ipcRing;

		proc::ipc::ring* ring;
	};

	class handle_catalogue {
		public:
		handle_catalogue& operator=(handle_catalogue&& other){
            this->id_gen = std::move(other.id_gen);
			this->catalogue = std::move(other.catalogue);

            return *this;
        }

		NODISCARD_ATTRIBUTE
		uint64_t push(handles::handle* handle){
			auto id = id_gen.id();
			catalogue.push_back(id, handle);

			return id;
		}

		template<typename T>
		T* get(uint64_t id){
			auto* handle = catalogue[id];
			if(!handle)
				return nullptr;
			
			ASSERT(handle->type == T::default_type);
			return static_cast<T*>(handle);
		}

		private:
		misc::id_generator id_gen;
		types::hash_map<uint64_t, handles::handle*, types::nop_hasher<uint64_t>> catalogue;
	};
} // namespace handles




#endif