#pragma once
#include <typeindex>
#include "base/Assert.hpp"
#include "ComponentMetadata.hpp"
#include "IComponent.hpp"
#include "EASTL/array.h"
#include "EASTL/fixed_hash_map.h"

#ifndef SPITE_TEST
#include "GeneratedComponentCount.hpp"
#endif

namespace spite
{
#ifdef SPITE_TEST
    constexpr ComponentID MAX_COMPONENTS = 100;
#endif


	class SharedComponentManager;

	struct TEMPCOMPONENT: IComponent
	{
	};

	class SharedComponentRegistryBridge
	{
		SharedComponentManager* m_sharedComponentManager = nullptr;

	public:
		explicit SharedComponentRegistryBridge(SharedComponentManager* sharedComponentManager);

		void destroyHandle(SharedComponentHandle handle) const;
	};

	class ComponentMetadataInitializer;

	class ComponentMetadataRegistry
	{
	private:
		eastl::fixed_hash_map<std::type_index, ComponentID, MAX_COMPONENTS, MAX_COMPONENTS + 1, false,std::hash<std::type_index>> m_typeToId;
		eastl::array<ComponentMetadata, MAX_COMPONENTS> m_idToMetadata;
		ComponentID m_nextComponentId = 1;

		friend ComponentMetadataInitializer;

	public:

		ComponentID getComponentId(const std::type_index& typeIndex) const;
		const ComponentMetadata& getMetadata(const std::type_index& typeIndex) const;
		const ComponentMetadata& getMetadata(ComponentID id) const;
	};

	class ComponentMetadataInitializer
	{
		ComponentMetadataRegistry* m_registry;
		SharedComponentRegistryBridge* m_sharedComponentManager;

		template <typename T>
		static void destructComponent(void* ptr, void* userData);

		// Destructor for SharedComponent<T> handles
		template <t_shared_handle T>
		static void destructSharedHandle(void* ptr, void* userData);

		// Move and destroy for regular components
		template <typename T>
		static void moveAndDestroyComponent(void* dest, void* src);

	public:
		ComponentMetadataInitializer(ComponentMetadataRegistry* registry, SharedComponentRegistryBridge* sharedManager);

		template <t_component T>
		void registerComponent();
	};

	template <typename T>
	void ComponentMetadataInitializer::destructComponent(void* ptr, void* userData)
	{
		static_cast<T*>(ptr)->~T();
	}

	template <t_shared_handle T>
	void ComponentMetadataInitializer::destructSharedHandle(void* ptr, void* userData)
	{
		auto* handleComponent = static_cast<T*>(ptr);
		auto* manager = static_cast<SharedComponentRegistryBridge*>(userData);
		manager->destroyHandle(handleComponent->handle);
	}

	template <typename T>
	void ComponentMetadataInitializer::moveAndDestroyComponent(void* dest, void* src)
	{
		new(dest) T(std::move(*static_cast<T*>(src)));
		static_cast<T*>(src)->~T();
	}

	template <t_component T>
	void ComponentMetadataInitializer::registerComponent()
	{
		std::type_index typeIndex(typeid(T));

		ComponentID newId = m_registry->m_nextComponentId++;
#ifndef SPITE_TEST
		SASSERT(newId < MAX_COMPONENTS)
#endif

		ComponentMetadata::DestructorFn destructorFn = nullptr;
		void* destructorUserData = nullptr;
		ComponentMetadata::MoveAndDestroyFn moveAndDestroyFn = nullptr;
		bool isTriviallyRelocatable;

		if constexpr (t_shared_handle<T>)
		{
			// This is a SharedComponent<T> handle. Assign the special destructor and user data.
			SASSERT(m_sharedComponentManager != nullptr)
			destructorFn = &destructSharedHandle<T>;
			destructorUserData = m_sharedComponentManager;
			// Handles are always trivially relocatable (they are just a struct with a handle).
			isTriviallyRelocatable = true;
		}
		else
		{
			// Regular component logic
			isTriviallyRelocatable = std::is_trivially_move_constructible_v<T> && std::is_trivially_destructible_v<
				T>;

			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				destructorFn = &destructComponent<T>;
			}

			if (!isTriviallyRelocatable)
			{
				moveAndDestroyFn = &moveAndDestroyComponent<T>;
			}
		}

		ComponentMetadata meta(newId,
		                       typeIndex,
		                       sizeof(T),
		                       alignof(T),
		                       isTriviallyRelocatable,
		                       destructorFn,
		                       destructorUserData,
		                       moveAndDestroyFn);

		m_registry->m_typeToId[typeIndex] = newId;
		m_registry->m_idToMetadata[newId] = meta;
	}
}
