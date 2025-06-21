#pragma once
#include <typeindex>

#include <EASTL/hash_map.h>

#include "IComponent.hpp"

#include "base/memory/HeapAllocator.hpp"

namespace spite
{
	struct ComponentMetadata
	{
		using DestructorFn = void (*)(void* componentPtr);
		// A function that move-constructs at 'dest' from 'src', destroys 'src'
		using MoveAndDestroyFn = void (*)(void* destPtr, void* srcPtr);

		std::type_index type;
		sizet size;
		sizet alignment;
		bool isTriviallyRelocatable = true;
		DestructorFn destructor = nullptr;
		// A function that move-constructs at 'dest' from 'src', destroys 'src'
		MoveAndDestroyFn moveAndDestroy = nullptr;

		ComponentMetadata(std::type_index type,
		                  sizet size,
		                  sizet alignment,
		                  bool isTriviallyRelocatable,
		                  DestructorFn destructorFn,
		                  MoveAndDestroyFn moveAndDestroyFn): type(type), size(size),
		                                                       alignment(alignment),
		                                                       isTriviallyRelocatable(
			                                                       isTriviallyRelocatable),
		                                                       destructor(destructorFn),
		                                                       moveAndDestroy(moveAndDestroyFn)
		{
		}
	};

	class ComponentMetadataRegistry
	{
	private:
		heap_unordered_map<std::type_index, ComponentMetadata> m_metadata;

		// Templated helper to generate a destructor function pointer for a given type.
		template <typename T>
		static void destructComponent(void* ptr)
		{
			static_cast<T*>(ptr)->~T();
		}

		// Templated helper to generate a function pointer that move-constructs a new
		// object at 'dest' from 'src', destroys 'src'
		template <typename T>
		static void moveAndDestroyComponent(void* dest, void* src)
		{
			new(dest) T(std::move(*static_cast<T*>(src)));
			static_cast<T*>(src)->~T();
		}

	public:
		explicit ComponentMetadataRegistry(const HeapAllocator& allocator) : m_metadata(allocator)
		{
		}

		template <t_component T>
		void registerComponent()
		{
			std::type_index typeIndex(typeid(T));
			if (m_metadata.count(typeIndex))
			{
				return;
			}
			// A type is "trivially relocatable" if its move operation is equivalent to a memcpy
			// and its moved-from state requires no destruction. This is true for types that are
			// both trivially move constructible and trivially destructible.
			bool isTriviallyRelocatable = std::is_trivially_move_constructible_v<T> &&
				std::is_trivially_destructible_v<T>;

			ComponentMetadata::MoveAndDestroyFn moveAndDestroyFn = nullptr;
			ComponentMetadata::DestructorFn destructorFn = nullptr;
			// If the type has a non-trivial destructor, store a pointer to our helper function.
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				destructorFn = &destructComponent<T>;
			}
			// If the type is not trivially relocatable, we need a proper move-and-destruct function.
			if (!isTriviallyRelocatable)
			{
				moveAndDestroyFn = &moveAndDestroyComponent<T>;
			}

			ComponentMetadata meta(typeIndex,
			                       sizeof(T),
			                       alignof(T),
			                       isTriviallyRelocatable,
			                       destructorFn,
			                       moveAndDestroyFn);

			m_metadata[typeIndex] = meta;
		}

		const ComponentMetadata& getMetadata(const std::type_index& typeIndex) const
		{
			auto it = m_metadata.find(typeIndex);
			SASSERTM(it != m_metadata.end(),
			         "Component of type %s is not registered\n",
			         typeIndex.name())
			return it->second;
		}
	};
}
