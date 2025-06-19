#pragma once
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include "base/memory/HeapAllocator.hpp"

#include "ecs/Aspect.hpp"
#include "ecs/ComponentMetadata.hpp"
#include "ecs/Core.hpp"

namespace spite
{
	class Archetype;

	constexpr sizet DEFAULT_CHUNK_CAPACITY = 128;

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
		      const HeapAllocator& allocator) : m_aspect(aspect), m_entities(allocator),
		                                        m_componentDataStarts(allocator),
		                                        m_componentArraysStorage(allocator), m_count(0),
		                                        m_capacity(capacity), m_allocator(allocator),
		                                        m_metadataRegistry(metadataRegistry)
		{
			m_entities.reserve(m_capacity);

			const auto& componentTypes = m_aspect->getTypes();
			m_componentDataStarts.reserve(componentTypes.size());
			m_componentArraysStorage.reserve(componentTypes.size());

			for (const auto& typeIndex : componentTypes)
			{
				ComponentMetadata meta = m_metadataRegistry->getMetadata(typeIndex);
				SASSERT(meta.size > 0)

				auto* byteArray = static_cast<std::byte*>(m_allocator.allocate(
					meta.size * m_capacity,
					meta.alignment));
				m_componentArraysStorage.push_back(byteArray);
				m_componentDataStarts.push_back(byteArray);
			}
		}

		~Chunk()
		{
			sizet idx = 0;
			const auto& componentTypes = m_aspect->getTypes();

			SASSERTM(componentTypes.size() == m_componentDataStarts.size(),
			         "Chunk has invalid size of component arrays\n")
			for (const auto& typeIndex : componentTypes)
			{
				const auto& meta = m_metadataRegistry->getMetadata(typeIndex);
				m_allocator.deallocate(m_componentDataStarts[idx], meta.size * m_capacity);
				++idx;
			}
		}

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;
		Chunk(Chunk&&) = delete;
		Chunk& operator=(Chunk&&) = delete;

		bool isFull() const { return m_count >= m_capacity; }
		bool isEmpty() const { return m_count == 0; }
		sizet getCount() const { return m_count; }
		sizet getCapacity() const { return m_capacity; }
		const Aspect& getAspect() const { return *m_aspect; }

		// Adds an entity ID and returns the index within the chunk where its components will be stored.
		// The caller is responsible for constructing/placing the component data.
		sizet addEntity(const Entity entity)
		{
			SASSERT(!isFull())
			m_entities.push_back(entity);
			// Component data slots are implicitly reserved.
			return m_count++;
		}

		// Removes an entity at a given index within this chunk.
		// Uses swap-and-pop: the last entity in the chunk takes the place of the removed one.
		// Returns the Entity ID that was moved into the `entityChunkIndex` slot (or Entity::undefined if last was removed).
		// Destructors for removedEntity should be called in Archetype
		Entity removeEntityAndSwap(const sizet entityChunkIndex)
		{
			SASSERT(entityChunkIndex < m_count)

			Entity swappedEntity = Entity::undefined();

			sizet lastEntityIndex = m_count - 1;

			if (entityChunkIndex != lastEntityIndex) // If not removing the last element
			{
				// Move data for the last entity into the slot of the removed entity
				m_entities[entityChunkIndex] = m_entities[lastEntityIndex];
				swappedEntity = m_entities[entityChunkIndex];

				int typeIdx = 0;
				for (const auto& typeIndex : m_aspect->getTypes())
				{
					const auto& meta = m_metadataRegistry->getMetadata(typeIndex);
					sizet componentSize = meta.size;
					std::byte* componentArray = m_componentDataStarts[typeIdx];

					std::byte* dest = componentArray + (entityChunkIndex * componentSize);
					std::byte* src = componentArray + (lastEntityIndex * componentSize);

					if (meta.isTriviallyRelocatable)
					{
						memcpy(dest, src, componentSize);
					}
					else
					{
						meta.moveAndDestroy(dest, src);
					}

					typeIdx++;
				}
			}

			m_entities.pop_back();
			m_count--;

			return swappedEntity;
		}

		// Get a raw uncasted ptr to the component data for a specific entity and component type.
		void* getComponentDataPtr(const sizet entityChunkIdx, const std::type_index targetType)
		{
			SASSERT(entityChunkIdx < m_count)

			const auto& componentTypes = m_aspect->getTypes();
			for (size_t i = 0; i < componentTypes.size(); ++i)
			{
				if (componentTypes[i] == targetType)
				{
					const auto& meta = m_metadataRegistry->getMetadata(targetType);
					std::byte* componentArrayStart = m_componentDataStarts[i];
					return componentArrayStart + (entityChunkIdx * meta.size);
				}
			}
			SASSERTM(false, "Component type %s is not in chunk's aspect\n", targetType.name())
			return nullptr;
		}

		// Get a pointer to the component data for a specific entity and component type.
		template <typename TComponent>
		TComponent* getComponent(const sizet entityChunkIndex)
		{
			std::type_index targetType = typeid(TComponent);
			return static_cast<TComponent*>(getComponentDataPtr(entityChunkIndex, targetType));
		}

		// Get a raw pointer to the array of components of a specific type.
		// The array contains `m_count` valid components.
		template <typename TComponent>
		TComponent* getComponentArray()
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

		Entity getEntity(const sizet entityChunkIndex) const
		{
			SASSERT(entityChunkIndex < m_count)
			return m_entities[entityChunkIndex];
		}

		const heap_vector<Entity>& getEntities() const
		{
			return m_entities;
		}
	};

	// An Archetype manages a collection of Chunks, all sharing the same Aspect.
	class Archetype
	{
		const Aspect* m_aspect;

		const ComponentMetadataRegistry* m_metadataRegistry;

		heap_vector<std::unique_ptr<Chunk>> m_chunks;
		HeapAllocator m_allocator;

		// Maps Entity ID to its location (chunkIdx, entityIdxInChunk)
		heap_unordered_map<Entity, eastl::pair<sizet, sizet>, Entity::hash> m_entityLocations;

	public:
		Archetype(const Aspect* aspect,
		          const ComponentMetadataRegistry* registry,
		          const HeapAllocator& allocator) : m_aspect(aspect), m_metadataRegistry(registry),
		                                            m_allocator(allocator)
		{
		}

		// Finds or creates a chunk with space, adds the entity, and returns its location.
		// second elem in pair is Entity's idx in provided chunk
		// The caller would then emplace/construct components into the returned pointers.
		eastl::pair<Chunk*, sizet> addEntity(const Entity entity)
		{
			Chunk* targetChunk = nullptr;
			sizet chunkIndex;
			for (chunkIndex = 0; chunkIndex < m_chunks.size(); ++chunkIndex)
			{
				if (!m_chunks[chunkIndex]->isFull())
				{
					targetChunk = m_chunks[chunkIndex].get();
					break;
				}
			}
			if (!targetChunk)
			{
				auto newChunk = std::make_unique<Chunk>(m_aspect,
				                                        DEFAULT_CHUNK_CAPACITY,
				                                        m_metadataRegistry,
				                                        m_allocator);
				targetChunk = newChunk.get();
				m_chunks.push_back(std::move(newChunk));
				chunkIndex = m_chunks.size() - 1; // Index of the newly added chunk
			}

			sizet indexInChunk = targetChunk->addEntity(entity);
			m_entityLocations[entity] = {chunkIndex, indexInChunk};
			return {targetChunk, indexInChunk};
		}

		void removeEntity(Entity entity)
		{
			auto it = m_entityLocations.find(entity);
			if (it == m_entityLocations.end())
			{
				SASSERTM(false, "Entity %llu not in this archetype for removal\n,",entity.id())
				return;
			}

			sizet chunkIdx = it->second.first;
			sizet entityIdxInChunk = it->second.second;

			Chunk* chunk = m_chunks[chunkIdx].get();

			const auto& componentTypes = m_aspect->getTypes();

			// Before swapping, call destructors for components of the entity being removed
			for (const auto& componentType : componentTypes)
			{
				const auto& meta = m_metadataRegistry->getMetadata(componentType);
				if (!meta.isTriviallyRelocatable)
				{
					void* componentPtr = chunk->getComponentDataPtr(
						entityIdxInChunk,
						componentType);
					meta.destructor(componentPtr);
				}
			}

			Entity swappedEntity = chunk->removeEntityAndSwap(entityIdxInChunk);

			m_entityLocations.erase(entity);
			if (swappedEntity != Entity::undefined())
			{
				// If an entity was swapped into the removed slot
				m_entityLocations[swappedEntity] = {chunkIdx, entityIdxInChunk};
			}

			// Optional: if chunk becomes empty and is not the last one, can consider compacting or adding to a free list.
		}

		const heap_vector<std::unique_ptr<Chunk>>& getChunks() const
		{
			return m_chunks;
		}

		eastl::pair<Chunk*, sizet> getEntityLocation(Entity entity) const
		{
			auto it = m_entityLocations.find(entity);
			if (it != m_entityLocations.end())
			{
				return {m_chunks[it->second.first].get(), it->second.second};
			}
			SASSERTM(false, "Entity %llu is not located in Archetype\n", entity.id())
			return {nullptr, static_cast<sizet>(-1)};
		}
	};

	// An ArchetypeManager 
	// map Aspect -> Archetype
	// class ArchetypeManager {
	// };
}
