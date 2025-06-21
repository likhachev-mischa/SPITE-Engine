#include "Archetype.hpp"

#include "ComponentMetadata.hpp"

namespace spite
{
	Archetype::Archetype(const Aspect* aspect,
	                     const ComponentMetadataRegistry* registry,
	                     const HeapAllocator& allocator): m_aspect(aspect),
	                                                      m_metadataRegistry(registry),
	                                                      m_allocator(allocator)
	{
	}

	eastl::pair<Chunk*, sizet> Archetype::addEntity(const Entity entity)
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

	void Archetype::removeEntity(Entity entity)
	{
		auto it = m_entityLocations.find(entity);
		if (it == m_entityLocations.end())
		{
			SASSERTM(false, "Entity %llu not in this archetype for removal\n,", entity.id())
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
				void* componentPtr = chunk->getComponentDataPtr(entityIdxInChunk, componentType);
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

	const heap_vector<std::unique_ptr<Chunk>>& Archetype::getChunks() const
	{
		return m_chunks;
	}

	eastl::pair<Chunk*, sizet> Archetype::getEntityLocation(Entity entity) const
	{
		auto it = m_entityLocations.find(entity);
		if (it != m_entityLocations.end())
		{
			return {m_chunks[it->second.first].get(), it->second.second};
		}
		SASSERTM(false, "Entity %llu is not located in Archetype\n", entity.id())
		return {nullptr, static_cast<sizet>(-1)};
	}

	Archetype* ArchetypeManager::getOrCreateArchetype(const Aspect& aspect)
	{
		auto it = m_archetypes.find(aspect);
		if (it != m_archetypes.end())
		{
			return it->second.get();
		}

		// Create new archetype
		auto newArchetype = std::make_unique<Archetype>(&aspect, m_metadataRegistry, m_allocator);
		Archetype* result = newArchetype.get();
		m_archetypes[aspect] = std::move(newArchetype);
		return result;
	}

	Archetype* ArchetypeManager::findArchetype(const Aspect& aspect) const
	{
		auto it = m_archetypes.find(aspect);
		if (it != m_archetypes.end())
		{
			return it->second.get();
		}
		return nullptr;
	}

	const heap_unordered_map<Aspect, std::unique_ptr<Archetype>>&
	ArchetypeManager::getArchetypes() const
	{
		return m_archetypes;
	}

	void ArchetypeManager::moveEntity(Entity entity,
	                                  const Aspect& fromAspect,
	                                  const Aspect& toAspect)
	{
		SASSERT(isEntityTracked(entity))
		Archetype* fromArchetype = findArchetype(fromAspect);
		Archetype* toArchetype = getOrCreateArchetype(toAspect);

		SASSERT(fromArchetype)
		SASSERT(toArchetype)

		// Get entity's current location
		auto [fromChunk, fromIndex] = fromArchetype->getEntityLocation(entity);

		// Add to new archetype
		auto [toChunk, toIndex] = toArchetype->addEntity(entity);

		// Copy compatible components between chunks
		copyCompatibleComponents(fromChunk, fromIndex, toChunk, toIndex, fromAspect, toAspect);

		// Remove from old archetype
		fromArchetype->removeEntity(entity);

		m_entityToArchetype[entity] = toArchetype;
	}

	void ArchetypeManager::removeEntity(Entity entity, const Aspect& aspect)
	{
		SASSERT(isEntityTracked(entity))
		Archetype* archetype = findArchetype(aspect);
		SASSERT(archetype)
		archetype->removeEntity(entity);

		m_entityToArchetype.erase(entity);
	}

	Archetype* ArchetypeManager::getEntityArchetype(Entity entity) const
	{
		SASSERT(isEntityTracked(entity))
		return m_entityToArchetype.at(entity);
	}

	heap_vector<Archetype*> ArchetypeManager::queryArchetypes(const Aspect& includeAspect,
	                                                          const Aspect& excludeAspect) const
	{
		heap_vector<Archetype*> result;

		for (const auto& [aspect, archetype] : m_archetypes)
		{
			if (aspect.contains(includeAspect) && !aspect.intersects(excludeAspect))
			{
				result.push_back(archetype.get());
			}
		}

		return result;
	}

	bool ArchetypeManager::isEntityTracked(Entity entity) const
	{
		return m_entityToArchetype.find(entity) != m_entityToArchetype.end();
	}

	void ArchetypeManager::copyCompatibleComponents(Chunk* fromChunk,
	                                                sizet fromIndex,
	                                                Chunk* toChunk,
	                                                sizet toIndex,
	                                                const Aspect& fromAspect,
	                                                const Aspect& toAspect)
	{
		const auto& commonTypes = fromAspect.getIntersection(toAspect);

		for (const auto& componentType : commonTypes)
		{
			// Only copy if the component exists in both aspects
			const auto& metadata = m_metadataRegistry->getMetadata(componentType);

			// Get source and destination pointers
			void* srcPtr = fromChunk->getComponentDataPtr(fromIndex, componentType);
			void* dstPtr = toChunk->getComponentDataPtr(toIndex, componentType);

			if (srcPtr && dstPtr)
			{
				if (metadata.isTriviallyRelocatable)
				{
					std::memcpy(dstPtr, srcPtr, metadata.size);
				}
				else
				{
					metadata.moveAndDestroy(dstPtr, srcPtr);
				}
			}
		}
	}
}
