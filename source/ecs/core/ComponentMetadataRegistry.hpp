#pragma once
#include "ComponentMetadata.hpp"
#include "IComponent.hpp"
#include "base/Platform.hpp"
#include <utility>
#include <type_traits>
#include <mutex>
#include <typeindex>

#include "base/Assert.hpp"
#include "base/CollectionAliases.hpp"


namespace spite
{
	class SharedComponentManager;

	// This context is passed to the policy function at runtime.
	class DestructionContext
	{
	private:
		SharedComponentManager* sharedManager;

	public:
		DestructionContext(SharedComponentManager* sharedManager): sharedManager(sharedManager)
		{
		}

		void destroySharedHandle(void* component) const;
	};

	namespace detail
	{
		// The default destruction policy: do nothing.
		inline void empty_destruction_policy(void* /*componentPtr*/, const DestructionContext& /*context*/)
		{
		}

		// Creates a single ComponentMetadata entry for a given component type.
		template <t_component T>
		ComponentMetadata create_metadata_for(ComponentID id)
		{
			// Start with the default no-op policy.
			ComponentMetadata::DestructionPolicyFn policyFn = &empty_destruction_policy;
			ComponentMetadata::MoveAndDestroyFn moveAndDestroyFn;

			// --- Overwrite Destruction Policy for Special Cases ---
			if constexpr (t_shared_handle<T>)
			{
				policyFn = [](void* c, const DestructionContext& ctx)
				{
					ctx.destroySharedHandle(c);
				};
			}
			else if constexpr (!std::is_trivially_destructible_v<T>)
			{
				policyFn = [](void* c, const DestructionContext&) { static_cast<T*>(c)->~T(); };
			}

			// --- Select Move Policy ---
			if constexpr (std::is_trivially_move_constructible_v<T> && std::is_trivially_destructible_v<T>)
			{
				// Most optimal path: raw memory copy.
				moveAndDestroyFn = [](void* dest, void* src)
				{
					memcpy(dest, src, sizeof(T));
				};
			}
			else
			{
				// Safe path for non-trivial types.
				moveAndDestroyFn = [](void* dest, void* src)
				{
					new(dest) T(std::move(*static_cast<T*>(src)));
					static_cast<T*>(src)->~T();
				};
			}

			return ComponentMetadata(
				id,
				sizeof(T),
				alignof(T),
				policyFn,
				moveAndDestroyFn
			);
		}
	}

	//Currently statically initialized within main world
	class ComponentMetadataRegistry
	{
	private:
		heap_vector<ComponentMetadata> m_idToMetadata;
		heap_unordered_map<std::type_index, ComponentID,std::hash<std::type_index>> m_typeToIdMap;

		HeapAllocator m_allocator;
		static ComponentMetadataRegistry* m_instance;

	public:
		ComponentMetadataRegistry(const HeapAllocator& allocator);

		static void init(HeapAllocator& allocator)
		{
			SASSERTM(!ComponentMetadataRegistry::m_instance, "ComponentMetadataRegistry is already initialized\n")
			ComponentMetadataRegistry::m_instance = allocator.new_object<ComponentMetadataRegistry>(allocator);
		}

		template <t_component T>
		static ComponentID registerComponent()
		{
			SASSERTM(ComponentMetadataRegistry::m_instance, "ComponentMetadataRegistry is not initialized\n")
			const auto typeIndex = std::type_index(typeid(T));
			auto& typeToIdMap = ComponentMetadataRegistry::m_instance->m_typeToIdMap;
			auto it = typeToIdMap.find(typeIndex);
			if (it != typeToIdMap.end())
			{
				return it->second; // Already registered
			}

			auto& idToMetadata = ComponentMetadataRegistry::m_instance->m_idToMetadata;
			const ComponentID newId = static_cast<ComponentID>(idToMetadata.size());
			idToMetadata.push_back(detail::create_metadata_for<T>(newId));
			typeToIdMap[typeIndex] = newId;
			SDEBUG_LOG("ComponentMetadata for %s is registered with %u id\n", typeIndex.name(), newId)
			return newId;
		}

		template <t_component T>
		static ComponentID getComponentId()
		{
			static const ComponentID id = registerComponent<T>();
			return id;
		}

		static const ComponentMetadata& getMetadata(ComponentID id)
		{
			SASSERTM(ComponentMetadataRegistry::m_instance, "ComponentMetadataRegistry is not initialized\n")
			auto& idToMetadata = ComponentMetadataRegistry::m_instance->m_idToMetadata;
			SASSERTM(id < idToMetadata.size(),"Id %u is out of ComponentMetadataRegistry bounds",id)
			return idToMetadata[id];
		}

		template <t_component T>
		static const ComponentMetadata& getMetadata()
		{
			static const ComponentMetadata metadata = getMetadata(getComponentId<T>());
			return metadata;
		}

		static sizet getRegisteredComponentCount()
		{
			SASSERTM(ComponentMetadataRegistry::m_instance, "ComponentMetadataRegistry is not initialized\n")
			auto& idToMetadata = ComponentMetadataRegistry::m_instance->m_idToMetadata;
			return idToMetadata.size();
		}

		static void destroy()
		{
			SASSERTM(ComponentMetadataRegistry::m_instance, "ComponentMetadataRegistry is not initialized\n")
			ComponentMetadataRegistry::m_instance->m_allocator.delete_object(ComponentMetadataRegistry::m_instance);
		}
	};
}
