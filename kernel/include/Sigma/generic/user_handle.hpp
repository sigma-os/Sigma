#ifndef SIGMA_GENERIC_USER_HANDLE_H
#define SIGMA_GENERIC_USER_HANDLE_H

#include <Sigma/common.h>
#include <Sigma/types/hash_map.hpp>
#include <Sigma/generic/virt.hpp>

#include <klibcxx/utility.hpp>

#include <Sigma/generic/event.hpp>

namespace generic::handles
{
	enum class handle_type {VCpu, VSpace, Irq};

	struct handle {
		handle(handle_type type): type{type} {}
		handle_type type;

		virtual ~handle() {}
	};

	struct vcpu_handle : public handle {
		vcpu_handle(generic::virt::vspace* space): handle{handle_type::VCpu}, cpu{space} {}
		
		static constexpr handle_type default_type = handle_type::VCpu;

		generic::virt::vcpu cpu;
	};

	struct vspace_handle : public handle {
		vspace_handle(): handle{handle_type::VSpace}, space{} {}
	
		static constexpr handle_type default_type = handle_type::VSpace;

		generic::virt::vspace space;
	};

	struct irq_handle : public handle {
		irq_handle(uint8_t vector): handle{handle_type::Irq}, vector{vector}, event{} {}

		static constexpr handle_type default_type = handle_type::Irq;

		uint8_t vector;
		generic::event event;
	};

	class handle_catalogue {
		public:
		uint64_t push(handles::handle* handle){
			auto id = id_gen.id();
			catalogue.push_back(id, handle);

			return id;
		}

		template<typename T>
		T* get(uint64_t id){
			auto* handle = catalogue[id];
			ASSERT(handle->type == T::default_type);

			return static_cast<T*>(handle);
		}

		private:
		misc::id_generator id_gen;
		//types::linked_list<std::pair<uint64_t, handles::handle*>> catalogue;
		types::hash_map<uint64_t, handles::handle*, types::nop_hasher<uint64_t>> catalogue;
	};
} // namespace handles




#endif