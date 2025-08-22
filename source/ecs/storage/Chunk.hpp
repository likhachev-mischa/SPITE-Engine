#pragma once
#include <bitset>

#include <EASTL/span.h>

#include "ecs/core/Entity.hpp"

#include "base/CollectionAliases.hpp"
#include "base/memory/HeapAllocator.hpp"

#include "ecs/core/ComponentMetadataRegistry.hpp"
#include "ecs/storage/Aspect.hpp"

namespace spite
{
	class ComponentMetadataRegistry;
	constexpr sizet DEFAULT_CHUNK_CAPACITY = 32;
	constexpr sizet DEFAULT_COMPONENTS_INLINE_CAPACITY = 8;

	class Chunk
	{
		static constexpr sizet CAPACITY = DEFAULT_CHUNK_CAPACITY;

	private:
		const Aspect* m_aspect;
		sizet m_count;
		HeapAllocator& m_allocator;

		// A single block of memory for all component arrays.
		std::byte* m_storageBlock;

		eastl::array<Entity, CAPACITY> m_entities;
		heap_sbo_vector<std::byte*, DEFAULT_COMPONENTS_INLINE_CAPACITY> m_componentDataStarts;

		// Per-component modification tracking bitsets.
		heap_sbo_vector<std::bitset<CAPACITY>, DEFAULT_COMPONENTS_INLINE_CAPACITY>
		m_modifiedBitsets;
		heap_sbo_vector<std::bitset<CAPACITY>, DEFAULT_COMPONENTS_INLINE_CAPACITY> m_enabledBitsets;

	public:
		Chunk(const Aspect* aspect,
		      HeapAllocator& allocator);

		~Chunk();

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;

		Chunk(Chunk&& other) noexcept;

		Chunk& operator=(Chunk&& other) noexcept;

		[[nodiscard]] bool full() const;

		[[nodiscard]] bool empty() const;

		[[nodiscard]] sizet size() const;

		[[nodiscard]] sizet capacity() const;

		[[nodiscard]] const Aspect& aspect() const;

		//should be used for per-chunk iteration
		template <typename T>
		T* getComponents() const;

		//should be used for per-chunk iteration
		//marks components as modified
		template <typename T>
		T* getComponents();

		// Adds an entity ID and returns the index within the chunk where its components will be stored.
		// The caller is responsible for constructing/placing the component data.
		sizet addEntity(const Entity entity);

		// Removes an entity at a given index within this chunk.
		// Uses swap-and-pop: the last entity in the chunk takes the place of the removed one.
		// Returns the Entity ID that was moved into the `entityChunkIndex` slot (or Entity::undefined if last was removed).
		// Destructors for removedEntity should be called in Archetype
		Entity removeEntityAndSwap(const sizet entityChunkIndex);

		[[nodiscard]] Entity entity(const sizet entityChunkIndex) const;

		[[nodiscard]] eastl::span<const Entity> entities() const;

		[[nodiscard]] bool wasModifiedLastFrameByIndex(const sizet componentIndexInChunk,
		                                               const sizet entityChunkIndex) const;

		void resetModificationTracking();

		// Gets a raw pointer to an entity's component data using a pre-calculated index. (O(1) access)
		void* getComponentDataPtrByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);

		const void* getComponentDataPtrByIndex(sizet componentIndexInChunk,
		                                       sizet entityIndexInChunk) const;

		// Marks a component as modified using a pre-calculated index. (O(1) access)
		void markModifiedByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);

		void enableComponentByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);

		void disableComponentByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);

		[[nodiscard]] bool isComponentEnabledByIndex(sizet componentIndexInChunk,
		                                             sizet entityIndexInChunk) const;
	};

	template <typename T>
	T* Chunk::getComponents() const
	{
		auto id = ComponentMetadataRegistry::getComponentId<T>();
		SASSERT(m_aspect->contains(id))

		const auto& aspectIds = m_aspect->getComponentIds();
		sizet componentIdx = 0;

		for (sizet i = 0; i < aspectIds.size(); ++i)
		{
			if (aspectIds[i] == id)
			{
				componentIdx = i;
				break;
			}
		}

		return reinterpret_cast<T*>(m_componentDataStarts[componentIdx]);
	}

	template <typename T>
	T* Chunk::getComponents()
	{
		auto id = ComponentMetadataRegistry::getComponentId<T>();
		SASSERT(m_aspect->contains(id))

		const auto& aspectIds = m_aspect->getComponentIds();
		sizet componentIdx = 0;

		for (sizet i = 0; i < aspectIds.size(); ++i)
		{
			if (aspectIds[i] == id)
			{
				componentIdx = i;
				break;
			}
		}

		m_modifiedBitsets[componentIdx].set();

		return reinterpret_cast<T*>(m_componentDataStarts[componentIdx]);
	}
}
