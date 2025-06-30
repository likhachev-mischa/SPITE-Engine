#pragma once
#include <bitset>

#include <EASTL/array.h>
#include <EASTL/span.h>

#include "Entity.hpp"

#include "base/memory/HeapAllocator.hpp"

#include "ecs/Aspect.hpp"

namespace spite
{
	class ComponentMetadataRegistry;
	constexpr sizet DEFAULT_CHUNK_CAPACITY = 64;
	constexpr sizet MAX_COMPONENTS_IN_ASPECT = 32;


	class Chunk
	{
		static constexpr sizet CAPACITY = DEFAULT_CHUNK_CAPACITY;
		static constexpr sizet MAX_COMPONENT_TYPES = MAX_COMPONENTS_IN_ASPECT;

	private:
		Aspect m_aspect;
		sizet m_count;
		const ComponentMetadataRegistry* m_metadataRegistry;
		HeapAllocator& m_allocator;

		// A single block of memory for all component arrays.
		std::byte* m_storageBlock;

		eastl::array<Entity, CAPACITY> m_entities;
		eastl::array<std::byte*, MAX_COMPONENT_TYPES> m_componentDataStarts{};

		// Per-component modification tracking bitsets.
		eastl::array<std::bitset<CAPACITY>, MAX_COMPONENT_TYPES> m_modifiedBitsets;

	public:
		Chunk(Aspect aspect,
		      const ComponentMetadataRegistry* metadataRegistry,
		      HeapAllocator& allocator);

		~Chunk();

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;

		Chunk(Chunk&& other) noexcept;

		Chunk& operator=(Chunk&& other) noexcept;

		[[nodiscard]] bool full() const;

		[[nodiscard]] bool empty() const;

		[[nodiscard]] sizet count() const;

		[[nodiscard]] sizet capacity() const;

		[[nodiscard]] const Aspect& aspect() const;

		// Adds an entity ID and returns the index within the chunk where its components will be stored.
		// The caller is responsible for constructing/placing the component data.
		sizet addEntity(const Entity entity);

		// Removes an entity at a given index within this chunk.
		// Uses swap-and-pop: the last entity in the chunk takes the place of the removed one.
		// Returns the Entity ID that was moved into the `entityChunkIndex` slot (or Entity::undefined if last was removed).
		// Destructors for removedEntity should be called in Archetype
		Entity removeEntityAndSwap(const sizet entityChunkIndex);


		// marks component as modified
		template <typename TComponent>
		TComponent& component(const sizet entityChunkIdx);

		// does not mark component as modified
		template <typename TComponent>
		const TComponent& component(const sizet entityChunkIndex) const;

		// Get a raw pointer to the array of components of a specific type.
		// The array contains `m_count` valid components.
		template <typename TComponent>
		TComponent* componentArray();

		template <typename TComponent>
		const TComponent* componentArray() const;

		[[nodiscard]] Entity entity(const sizet entityChunkIndex) const;

		[[nodiscard]] eastl::span<const Entity> entities() const;

		template <typename TComponent>
		[[nodiscard]] bool wasModifiedLastFrame(const sizet entityChunkIndex) const;

		[[nodiscard]] bool wasModifiedLastFrame(const sizet entityChunkIndex,
		                                        const std::type_index targetType) const;

		const std::bitset<CAPACITY>* getModifiedBitset(std::type_index type) const;

		void resetModificationTracking();

		// Get a raw uncasted ptr to the component data for a specific entity and component type.
		// Marks component as modified
		[[nodiscard]] void* componentDataPtr(const sizet entityChunkIdx,
		                                     const std::type_index targetType);

		// Get a raw uncasted ptr to the component data for a specific entity and component type.
		// Does not mark component as modified
		[[nodiscard]] const void* componentDataPtr(const sizet entityChunkIdx,
		                                           const std::type_index targetType) const;

		// Gets a raw pointer to an entity's component data using a pre-calculated index. (O(1) access)
		void* getComponentDataPtrByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);

		const void* getComponentDataPtrByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk) const;

		// Marks a component as modified using a pre-calculated index. (O(1) access)
		void markModifiedByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);
	};
}
