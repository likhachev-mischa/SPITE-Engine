#include "Archetype.hpp"

#include "ComponentMetadata.hpp"

#include "base/memory/ScratchAllocator.hpp"

namespace spite
{
	Archetype::Archetype(Aspect aspect,
	                     const ComponentMetadataRegistry* registry,
	                     HeapAllocator& allocator): m_aspect(std::move(aspect)),
	                                                m_componentIndexMap(allocator),
	                                                m_metadataRegistry(registry),
	                                                m_freeChunks(allocator),
	                                                m_firstNonFullChunkIdx(0),
	                                                m_allocator(allocator)
	{
		const auto& types = m_aspect.getTypes();
		for (int i = 0, size = static_cast<int>(types.size()); i < size; ++i)
		{
			m_componentIndexMap[types[i]] = i;
		}
	}

	eastl::pair<Chunk*, sizet> Archetype::addEntity(const Entity entity)
	{
		Chunk* targetChunk = nullptr;
		sizet chunkIndex;

		// Start search from the last known non-full chunk.
		if (m_firstNonFullChunkIdx < m_chunks.size() && !m_chunks[m_firstNonFullChunkIdx]->full())
		{
			targetChunk = m_chunks[m_firstNonFullChunkIdx].get();
			chunkIndex = m_firstNonFullChunkIdx;
		}
		else
		{
			// Otherwise, find the next non-full chunk and update the index.
			bool found = false;
			for (chunkIndex = 0; chunkIndex < m_chunks.size(); ++chunkIndex)
			{
				if (!m_chunks[chunkIndex]->full())
				{
					targetChunk = m_chunks[chunkIndex].get();
					m_firstNonFullChunkIdx = chunkIndex;
					found = true;
					break;
				}
			}
			if (!found) targetChunk = nullptr;
		}

		if (!targetChunk)
		{
			if (!m_freeChunks.empty())
			{
				m_chunks.push_back(std::move(m_freeChunks.back()));
				m_freeChunks.pop_back();
				chunkIndex = m_chunks.size() - 1;
				targetChunk = m_chunks[chunkIndex].get();
				m_firstNonFullChunkIdx = chunkIndex;
			}
			else
			{
				auto newChunk = std::make_unique<Chunk>(m_aspect, m_metadataRegistry, m_allocator);
				targetChunk = newChunk.get();
				m_chunks.push_back(std::move(newChunk));
				chunkIndex = m_chunks.size() - 1;
				m_firstNonFullChunkIdx = chunkIndex; // The new chunk is now the first non-full one.
			}
		}

		sizet indexInChunk = targetChunk->addEntity(entity);
		m_entityLocations[entity] = {chunkIndex, indexInChunk};
		return {targetChunk, indexInChunk};
	}

	scratch_vector<eastl::pair<Chunk*, sizet>> Archetype::addEntities(
		eastl::span<const Entity> entities)
	{
		if (entities.empty()) return {};

		auto locations = FrameScratchAllocator::makeVector<eastl::pair<Chunk*,sizet>>();
		locations.reserve(entities.size());
		m_entityLocations.reserve(m_entityLocations.size() + entities.size());

		sizet entitiesAdded = 0;
		sizet totalToAdd = entities.size();

		// 1. Fill existing chunks
		for (sizet chunkIdx = 0; chunkIdx < m_chunks.size() && entitiesAdded < totalToAdd; ++
		     chunkIdx)
		{
			Chunk* chunk = m_chunks[chunkIdx].get();
			while (!chunk->full() && entitiesAdded < totalToAdd)
			{
				const Entity& entity = entities[entitiesAdded];
				sizet indexInChunk = chunk->addEntity(entity);
				auto location = eastl::make_pair(chunk, indexInChunk);
				m_entityLocations[entity] = {chunkIdx, indexInChunk};
				locations.push_back(location);
				entitiesAdded++;
			}
		}

		// 2. Allocate and fill new chunks
		if (entitiesAdded < totalToAdd)
		{
			sizet remaining = totalToAdd - entitiesAdded;
			sizet numNewChunks = (remaining + DEFAULT_CHUNK_CAPACITY - 1) / DEFAULT_CHUNK_CAPACITY;
			m_chunks.reserve(m_chunks.size() + numNewChunks);

			for (sizet i = 0; i < numNewChunks; ++i)
			{
				auto newChunk = std::make_unique<Chunk>(m_aspect, m_metadataRegistry, m_allocator);
				m_chunks.push_back(std::move(newChunk));
			}

			for (sizet chunkIdx = m_chunks.size() - numNewChunks; chunkIdx < m_chunks.size() &&
			     entitiesAdded < totalToAdd; ++chunkIdx)
			{
				Chunk* chunk = m_chunks[chunkIdx].get();
				while (!chunk->full() && entitiesAdded < totalToAdd)
				{
					const Entity& entity = entities[entitiesAdded];
					sizet indexInChunk = chunk->addEntity(entity);
					auto location = eastl::make_pair(chunk, indexInChunk);
					m_entityLocations[entity] = {chunkIdx, indexInChunk};
					locations.push_back(location);
					entitiesAdded++;
				}
			}
		}
		return locations;
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

		const auto& componentTypes = m_aspect.getTypes();

		// Before swapping, call destructors for components of the entity being removed
		for (const auto& componentType : componentTypes)
		{
			const auto& meta = m_metadataRegistry->getMetadata(componentType);
			if (meta.destructor)
			{
				void* componentPtr = chunk->getComponentDataPtrByIndex(
					getComponentIndex(componentType),
					entityIdxInChunk);
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

		// If chunk becomes empty, move it to the free list
		if (chunk->empty())
		{
			const sizet lastChunkIdx = m_chunks.size() - 1;
			// Move the now-empty chunk to the free list.
			m_freeChunks.push_back(std::move(m_chunks[chunkIdx]));

			// If the empty chunk was not the last one, swap the last chunk into its place
			// to keep the active chunk vector contiguous.
			if (chunkIdx != lastChunkIdx)
			{
				// Move the last chunk into the now-empty slot
				m_chunks[chunkIdx] = std::move(m_chunks[lastChunkIdx]);

				// Update the locations for all entities in the chunk that was just moved.
				Chunk* movedChunk = m_chunks[chunkIdx].get();
				for (sizet i = 0, size = movedChunk->count(); i < size; ++i)
				{
					Entity updEntity = movedChunk->entity(i);
					if (updEntity != Entity::undefined())
					{
						m_entityLocations[updEntity].first = chunkIdx;
					}
				}
			}

			// Remove the last element (which is now either a duplicate pointer or the moved-from empty chunk)
			m_chunks.pop_back();

			// If the removed chunk was the one we were tracking as potentially non-full, we need to reset our thinking.
			if (m_firstNonFullChunkIdx == chunkIdx || m_firstNonFullChunkIdx == lastChunkIdx)
			{
				m_firstNonFullChunkIdx = 0; // Reset to a safe value.
			}
		}
	}

	void Archetype::removeEntities(eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;

		auto toRemoveByChunk= FrameScratchAllocator::makeMap<Chunk*,
			scratch_vector<sizet>>();

		// Group entities by chunk
		for (const auto& entity : entities)
		{
			auto it = m_entityLocations.find(entity);
			if (it != m_entityLocations.end())
			{
				Chunk* chunk = m_chunks[it->second.first].get();
				if (toRemoveByChunk.find(chunk) == toRemoveByChunk.end())
				{
					toRemoveByChunk.emplace(chunk, FrameScratchAllocator::makeVector<sizet>());
				}
				toRemoveByChunk[m_chunks[it->second.first].get()].push_back(it->second.second);
			}
		}

		for (auto& pair : toRemoveByChunk)
		{
			Chunk* chunk = pair.first;
			scratch_vector<sizet>& indices = pair.second;

			// Sort indices descending to safely use swap-and-pop
			eastl::sort(indices.begin(), indices.end(), eastl::greater<sizet>());

			const auto& componentTypes = m_aspect.getTypes();
			for (sizet indexToRemove : indices)
			{
				// Call destructors
				for (const auto& componentType : componentTypes)
				{
					const auto& meta = m_metadataRegistry->getMetadata(componentType);
					if (meta.destructor)
					{
						void* componentPtr = chunk->getComponentDataPtrByIndex(
							getComponentIndex(componentType),
							indexToRemove);
						meta.destructor(componentPtr);
					}
				}

				Entity removedEntity = chunk->entity(indexToRemove);
				Entity swappedEntity = chunk->removeEntityAndSwap(indexToRemove);

				m_entityLocations.erase(removedEntity);
				if (swappedEntity != Entity::undefined())
				{
					m_entityLocations[swappedEntity].second = indexToRemove;
				}
			}
		}
	}

	const heap_vector<std::unique_ptr<Chunk>>& Archetype::getChunks() const
	{
		return m_chunks;
	}

	const Aspect& Archetype::getAspect() const
	{
		return m_aspect;
	}

	int Archetype::getComponentIndex(const std::type_index& type) const
	{
		auto it = m_componentIndexMap.find(type);
		if (it != m_componentIndexMap.end())
		{
			return it->second;
		}
		return -1;
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

	bool Archetype::isEmpty() const
	{
		for (const auto& chunk : m_chunks)
		{
			if (!chunk->empty())
			{
				return false;
			}
		}
		return true;
	}

	ArchetypeManager::ArchetypeManager(const ComponentMetadataRegistry* registry,
	                                   const HeapAllocator& allocator): m_aspectRegistry(),
		m_metadataRegistry(registry), m_allocator(allocator), m_entityToArchetype(allocator)
	{
	}

	Archetype* ArchetypeManager::getOrCreateArchetype(const Aspect& aspect)
	{
		auto it = m_archetypes.find(aspect);
		if (it != m_archetypes.end())
		{
			return it->second.get();
		}

		m_aspectRegistry.addAspect(aspect);
		auto newArchetype = std::make_unique<Archetype>(aspect, m_metadataRegistry, m_allocator);
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

	void ArchetypeManager::addEntities(const Aspect& aspect, eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;
		Archetype* archetype = getOrCreateArchetype(aspect);
		archetype->addEntities(entities);
		for (const auto& entity : entities)
		{
			m_entityToArchetype[entity] = archetype;
		}
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

		auto [fromChunk, fromIndex] = fromArchetype->getEntityLocation(entity);

		//this marks the new entity's components as modified
		auto [toChunk, toIndex] = toArchetype->addEntity(entity);
		//so here it is safe to simply memcpy them
		copyCompatibleComponents(fromChunk, fromIndex, toChunk, toIndex, fromAspect, toAspect);

		fromArchetype->removeEntity(entity);

		m_entityToArchetype[entity] = toArchetype;
	}

	void ArchetypeManager::moveEntities(const Aspect& toAspect, eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;

		Archetype* toArchetype = getOrCreateArchetype(toAspect);
		auto groups = FrameScratchAllocator::makeMap<Archetype*,
			scratch_vector<Entity>>();

		for (const auto& entity : entities)
		{
			SASSERT(isEntityTracked(entity))
			groups[m_entityToArchetype.at(entity)].push_back(entity);
		}

		for (auto const& [fromArchetype, entityGroup] : groups)
		{
			if (fromArchetype != toArchetype)
			{
				moveEntitiesBetweenArchetypes(fromArchetype, toArchetype, entityGroup);
			}
		}

		for (const auto& entity : entities)
		{
			m_entityToArchetype[entity] = toArchetype;
		}
	}

	void ArchetypeManager::removeEntity(Entity entity, const Aspect& aspect)
	{
		SASSERT(isEntityTracked(entity))
		Archetype* archetype = findArchetype(aspect);
		SASSERT(archetype)
		archetype->removeEntity(entity);

		m_entityToArchetype.erase(entity);
	}

	void ArchetypeManager::removeEntities(eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;

		auto groups = FrameScratchAllocator::makeMap<Archetype*,
			scratch_vector<Entity>>();
		for (const auto& entity : entities)
		{
			auto it = m_entityToArchetype.find(entity);
			if (it != m_entityToArchetype.end())
			{
				groups[it->second].push_back(entity);
			}
		}

		for (auto const& [archetype, entityGroup] : groups)
		{
			archetype->removeEntities(entityGroup);
		}

		for (const auto& entity : entities)
		{
			m_entityToArchetype.erase(entity);
		}
	}

	const Archetype& ArchetypeManager::getEntityArchetype(Entity entity) const
	{
		SASSERT(isEntityTracked(entity))
		return *m_entityToArchetype.at(entity);
	}

	const Aspect& ArchetypeManager::getEntityAspect(Entity entity) const
	{
		return getEntityArchetype(entity).getAspect();
	}

	heap_vector<Archetype*> ArchetypeManager::queryArchetypes(const Aspect& includeAspect,
	                                                          const Aspect& excludeAspect) const
	{
		heap_vector<Archetype*> result;
		auto* rootNode = m_aspectRegistry.getNode(includeAspect);
		if (!rootNode)
		{
			return result;
		}

		auto descendantNodes = m_aspectRegistry.getDescendants(includeAspect);
		descendantNodes.push_back(rootNode);

		for (auto* node : descendantNodes)
		{
			if (!node->aspect.intersects(excludeAspect))
			{
				auto* archetype = findArchetype(node->aspect);
				if (archetype)
				{
					result.push_back(archetype);
				}
			}
		}

		return result;
	}

	heap_vector<Archetype*> ArchetypeManager::queryNonEmptyArchetypes(const Aspect& includeAspect,
		const Aspect& excludeAspect) const
	{
		heap_vector<Archetype*> result;
		auto* rootNode = m_aspectRegistry.getNode(includeAspect);
		if (!rootNode)
		{
			return result;
		}

		auto descendantNodes = m_aspectRegistry.getDescendants(includeAspect);
		descendantNodes.push_back(rootNode);

		for (auto* node : descendantNodes)
		{
			if (!node->aspect.intersects(excludeAspect))
			{
				auto* archetype = findArchetype(node->aspect);
				if (archetype && !archetype->isEmpty())
				{
					result.push_back(archetype);
				}
			}
		}

		return result;
	}

	void ArchetypeManager::resetAllModificationTracking()
	{
		for (const auto& [aspect, archetype] : m_archetypes)
		{
			for (const auto& chunk : archetype->getChunks())
			{
				chunk->resetModificationTracking();
			}
		}
	}

	bool ArchetypeManager::isEntityTracked(Entity entity) const
	{
		return m_entityToArchetype.find(entity) != m_entityToArchetype.end();
	}

	void ArchetypeManager::moveEntitiesBetweenArchetypes(Archetype* from,
	                                                     Archetype* to,
	                                                     eastl::span<const Entity> entities)
	{
		auto newLocations = to->addEntities(entities);
		auto newLocationMap = FrameScratchAllocator::makeMap<Entity,
			eastl::pair<Chunk*, sizet>>();
		for (size_t i = 0; i < entities.size(); ++i)
		{
			newLocationMap[entities[i]] = newLocations[i];
		}

		for (const auto& entity : entities)
		{
			auto [fromChunk, fromIndex] = from->getEntityLocation(entity);
			auto [toChunk, toIndex] = newLocationMap.at(entity);
			copyCompatibleComponents(fromChunk,
			                         fromIndex,
			                         toChunk,
			                         toIndex,
			                         from->getAspect(),
			                         to->getAspect());
		}

		from->removeEntities(entities);
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
			void* srcPtr = fromChunk->componentDataPtr(fromIndex, componentType);
			void* dstPtr = toChunk->componentDataPtr(toIndex, componentType);

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
