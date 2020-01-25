#ifndef SIGMA_GENERIC_USER_HANDLE_H
#define SIGMA_GENERIC_USER_HANDLE_H

#include <Sigma/common.h>
#include <Sigma/types/linked_list.h>
#include <Sigma/generic/virt.hpp>

#include <klibcxx/utility.hpp>

namespace handles
{
	enum class handle_type {VCpu, VSpace};

	struct handle {
		handle(handle_type type): type{type} {}
		handle_type type;

		virtual ~handle() {}
	};

	struct vcpu_handle : public handle {
		vcpu_handle(virt::vspace* space): handle{handle_type::VCpu}, cpu{space} {}
		
		static constexpr handle_type default_type = handle_type::VCpu;

		virt::vcpu cpu;
	};

	struct vspace_handle : public handle {
		vspace_handle(): handle{handle_type::VSpace}, space{} {}
	
		static constexpr handle_type default_type = handle_type::VSpace;

		virt::vspace space;

	};

	class handle_catalogue {
		public:
		uint64_t push(handles::handle* handle){
			auto id = id_gen.id();
			catalogue.push_back({id, handle});

			return id;
		}

		template<typename T>
		T* get(uint64_t id){
			for(auto& pair : catalogue){
				if(pair.first == id){
					ASSERT(pair.second->type == T::default_type);

					return static_cast<T*>(pair.second);
				}
			}

			return nullptr;
		}

		private:
		misc::id_generator id_gen;
		types::linked_list<std::pair<uint64_t, handles::handle*>> catalogue;
	};
} // namespace handles




#endif