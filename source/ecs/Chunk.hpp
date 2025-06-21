#pragma once
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include "base/memory/HeapAllocator.hpp"

#include "ecs/Aspect.hpp"

namespace spite
{
	struct Entity;
	class ComponentMetadataRegistry;
	constexpr sizet DEFAULT_CHUNK_CAPACITY = 64;

	class Chunk
	{
	private:
		const Aspect* m_aspect; // The set of component types this chunk stores.
		// Stores the actual Entity IDs.
		heap_vector<Entity> m_entities;
		// Stores pointers to the beginning of each component array.
		// The order matches the order of type_index in m_aspect.getTypes().
		heap_vector<std::byte*> m_componentDataStarts;

		// Raw block of memory holding all component arrays contiguously.
		heap_vector<std::byte*> m_componentArraysStorage;

		sizet m_count; // Current number of active entities in this chunk.
		sizet m_capacity; // Maximum number of entities this chunk can hold.
		HeapAllocator m_allocator;

		const ComponentMetadataRegistry* m_metadataRegistry;

	public:
		Chunk(const Aspect* aspect,
		      sizet capacity,
		      const ComponentMetadataRegistry* metadataRegistry,
		      const HeapAllocator& allocator);

		~Chunk();

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;
		Chunk(Chunk&&) = delete;
		Chunk& operator=(Chunk&&) = delete;

		bool isFull() const;
		bool isEmpty() const;
		sizet getCount() const;
		sizet getCapacity() const;
		const Aspect& getAspect() const;

		// Adds an entity ID and returns the index within the chunk where its components will be stored.
		// The caller is responsible for constructing/placing the component data.
		sizet addEntity(const Entity entity);

		// Removes an entity at a given index within this chunk.
		// Uses swap-and-pop: the last entity in the chunk takes the place of the removed one.
		// Returns the Entity ID that was moved into the `entityChunkIndex` slot (or Entity::undefined if last was removed).
		// Destructors for removedEntity should be called in Archetype
		Entity removeEntityAndSwap(const sizet entityChunkIndex);

		// Get a raw uncasted ptr to the component data for a specific entity and component type.
		void* getComponentDataPtr(const sizet entityChunkIdx, const std::type_index targetType);

		// Get a pointer to the component data for a specific entity and component type.
		template <typename TComponent>
		TComponent* getComponent(const sizet entityChunkIndex);

		// Get a raw pointer to the array of components of a specific type.
		// The array contains `m_count` valid components.
		template <typename TComponent>
		TComponent* getComponentArray();

		Entity getEntity(const sizet entityChunkIndex) const;

		const heap_vector<Entity>& getEntities() const;
	};

	template <typename TComponent>
	TComponent* Chunk::getComponent(const sizet entityChunkIndex)
	{
		std::type_index targetType = typeid(TComponent);
		return static_cast<TComponent*>(getComponentDataPtr(entityChunkIndex, targetType));
	}

	template <typename TComponent>
	TComponent* Chunk::getComponentArray()
	{
		std::type_index targetType = typeid(TComponent);
		const auto& componentTypes = m_aspect->getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == targetType)
			{
				return reinterpret_cast<TComponent*>(m_componentDataStarts[i]);
			}
		}
		SASSERTM(false, "Component type %s is not in chunk's aspect\n", targetType.name())
		return nullptr;
	}
}
