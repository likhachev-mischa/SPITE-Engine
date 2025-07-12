#include "Archetype.hpp"

#include "ecs/core/ComponentMetadataRegistry.hpp"
#include "VersionManager.hpp"

#include "base/CollectionUtilities.hpp"
#include "base/memory/ScratchAllocator.hpp"

namespace spite
{
	Archetype::Archetype(const Aspect* aspect,
	                     const ComponentMetadataRegistry* registry,
	                     HeapAllocator& allocator): m_aspect(aspect),
	                                                m_componentIdToIndexMap(
		                                                makeHeapMap<
			                                                ComponentID, int>(allocator)),
	                                                m_metadataRegistry(registry),
	                                                m_chunks(makeHeapVector<std::unique_ptr<Chunk>>(allocator)),
	                                                m_freeChunks(
		                                                makeHeapVector<std::unique_ptr<Chunk>>(
			                                                allocator)), m_firstNonFullChunkIdx(0),
	                                                m_allocator(allocator),
	                                                m_entityLocations(
		                                                makeHeapMap<Entity, eastl::pair<sizet, sizet>, Entity::hash>(
			                                                allocator))
	{
		const auto& ids = m_aspect->getComponentIds();
		for (int i = 0, size = static_cast<int>(ids.size()); i < size; ++i)
		{
			m_componentIdToIndexMap[ids[i]] = i;
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
		auto locations = makeScratchVector<eastl::pair<
			Chunk*, sizet>>(FrameScratchAllocator::get());
		if (entities.empty()) return locations;

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

		const auto& componentIds = m_aspect->getComponentIds();

		// Before swapping, call destructors for components of the entity being removed
		for (const auto& componentId : componentIds)
		{
			const auto& meta = m_metadataRegistry->getMetadata(componentId);
			if (meta.destructor)
			{
				void* componentPtr = chunk->getComponentDataPtrByIndex(
					getComponentIndex(componentId),
					entityIdxInChunk);
				meta.destructor(componentPtr, meta.destructorUserData);
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

	void Archetype::removeEntities(eastl::span<const Entity> entities, const Aspect* skipDestructionAspect)
	{
		if (entities.empty()) return;

		auto marker = FrameScratchAllocator::get().get_scoped_marker();

		auto toRemoveByChunk = makeScratchMap<Chunk*, scratch_vector<sizet>>(
			FrameScratchAllocator::get());

		// Group entities by chunk
		for (const auto& entity : entities)
		{
			auto it = m_entityLocations.find(entity);
			if (it != m_entityLocations.end())
			{
				Chunk* chunk = m_chunks[it->second.first].get();
				auto groupIt = toRemoveByChunk.find(chunk);
				if (groupIt == toRemoveByChunk.end())
				{
					groupIt = toRemoveByChunk.emplace(chunk, makeScratchVector<sizet>(FrameScratchAllocator::get())).
					                          first;
				}
				groupIt->second.push_back(it->second.second);
			}
		}

		for (auto& pair : toRemoveByChunk)
		{
			Chunk* chunk = pair.first;
			scratch_vector<sizet>& indices = pair.second;

			// Sort indices descending to safely use swap-and-pop
			eastl::sort(indices.begin(), indices.end(), eastl::greater<sizet>());

			const auto& componentIds = m_aspect->getComponentIds();
			for (sizet indexToRemove : indices)
			{
				// Call destructors
				for (const auto& componentId : componentIds)
				{
					if (skipDestructionAspect && skipDestructionAspect->contains(componentId)) continue;

					const auto& meta = m_metadataRegistry->getMetadata(componentId);
					if (meta.destructor)
					{
						void* componentPtr = chunk->getComponentDataPtrByIndex(
							getComponentIndex(componentId),
							indexToRemove);
						meta.destructor(componentPtr, meta.destructorUserData);
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

	const Aspect& Archetype::aspect() const
	{
		return *m_aspect;
	}

	int Archetype::getComponentIndex(ComponentID id) const
	{
		auto it = m_componentIdToIndexMap.find(id);
		if (it != m_componentIdToIndexMap.end())
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
		// An archetype is empty if it has no chunks, or if all of its chunks are empty.
		if (m_chunks.empty()) return true;
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
	                                   const HeapAllocator& allocator, AspectRegistry* aspectRegistry,
	                                   VersionManager* versionManager): m_archetypes(
		                                                                    makeHeapMap<
			                                                                    Aspect, std::unique_ptr<Archetype>,
			                                                                    Aspect::hash>(allocator)),
	                                                                    m_aspectRegistry(aspectRegistry),
	                                                                    m_metadataRegistry(registry),
	                                                                    m_allocator(allocator),
	                                                                    m_versionManager(versionManager),
	                                                                    m_entityToArchetype(
		                                                                    makeHeapMap<
			                                                                    Entity, Archetype*, Entity::hash>(
			                                                                    allocator))
	{
	}

	Archetype* ArchetypeManager::getOrCreateArchetype(const Aspect& aspect)
	{
		auto it = m_archetypes.find(aspect);
		if (it != m_archetypes.end())
		{
			return it->second.get();
		}

		const Aspect* registeredAspect = m_aspectRegistry->addOrGetAspect(aspect);
		auto newArchetype = std::make_unique<Archetype>(registeredAspect,
		                                                m_metadataRegistry,
		                                                m_allocator);
		Archetype* result = newArchetype.get();
		m_archetypes[aspect] = std::move(newArchetype);

		// A new archetype is created, which is a structural change.
		m_versionManager->makeDirty(*registeredAspect);

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

		const bool wasEmpty = archetype->isEmpty();
		archetype->addEntities(entities);
		if (wasEmpty)
		{
			m_versionManager->makeDirty(archetype->aspect());
		}

		for (const auto& entity : entities)
		{
			m_entityToArchetype[entity] = archetype;
		}
	}

	void ArchetypeManager::addComponent(const Entity entity, eastl::span<const ComponentID> componentsToAdd)
	{
		modifyComponent<false>(entity, componentsToAdd);
	}

	void ArchetypeManager::addComponents(eastl::span<const Entity> entities,
	                                     eastl::span<const ComponentID> componentsToAdd)
	{
		modifyComponents<false>(entities, componentsToAdd);
	}

	void ArchetypeManager::removeComponent(const Entity entity, eastl::span<const ComponentID> componentsToRemove)
	{
		modifyComponent<true>(entity, componentsToRemove);
	}

	void ArchetypeManager::removeComponents(eastl::span<const Entity> entities,
	                                        eastl::span<const ComponentID> componentsToRemove)
	{
		modifyComponents<true>(entities, componentsToRemove);
	}

	void ArchetypeManager::moveEntity(Entity entity,
	                                  const Aspect& toAspect)
	{
		SASSERT(isEntityTracked(entity))
		Archetype* fromArchetype = m_entityToArchetype.at(entity);
		Archetype* toArchetype = getOrCreateArchetype(toAspect);

		SASSERT(fromArchetype)
		SASSERT(toArchetype)

		moveEntitiesBetweenArchetypes(fromArchetype, toArchetype, {&entity, 1});
	}

	void ArchetypeManager::moveEntities(const Aspect& toAspect, eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;

		Archetype* toArchetype = getOrCreateArchetype(toAspect);
		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto groups = makeScratchMap<Archetype*, scratch_vector<Entity>>(
			FrameScratchAllocator::get());

		for (const auto& entity : entities)
		{
			SASSERT(isEntityTracked(entity))
			Archetype* archetype = m_entityToArchetype.at(entity);
			auto it = groups.find(archetype);
			if (it == groups.end())
			{
				it = groups.emplace(archetype, makeScratchVector<Entity>(FrameScratchAllocator::get())).first;
			}
			it->second.push_back(entity);
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

	void ArchetypeManager::removeEntity(Entity entity)
	{
		auto& archetype = getEntityArchetypeInternal(entity);

		const bool wasEmpty = archetype.isEmpty();
		archetype.removeEntity(entity);
		const bool isNowEmpty = archetype.isEmpty();

		if (!wasEmpty && isNowEmpty)
		{
			m_versionManager->makeDirty(archetype.aspect());
		}

		m_entityToArchetype.erase(entity);
	}

	void ArchetypeManager::removeEntities(eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto groups = makeScratchMap<Archetype*, scratch_vector<Entity>>(
			FrameScratchAllocator::get());
		for (const auto& entity : entities)
		{
			auto it = m_entityToArchetype.find(entity);
			if (it != m_entityToArchetype.end())
			{
				auto groupIt = groups.find(it->second);
				if (groupIt == groups.end())
				{
					groupIt = groups.emplace(it->second, makeScratchVector<Entity>(FrameScratchAllocator::get())).first;
				}
				groupIt->second.push_back(entity);
			}
		}

		for (auto const& [archetype, entityGroup] : groups)
		{
			const bool wasEmpty = archetype->isEmpty();
			archetype->removeEntities(entityGroup);
			const bool isNowEmpty = archetype->isEmpty();

			if (!wasEmpty && isNowEmpty)
			{
				m_versionManager->makeDirty(archetype->aspect());
			}
		}

		for (const auto& entity : entities)
		{
			m_entityToArchetype.erase(entity);
		}
	}

	void ArchetypeManager::addEntity(const Aspect& aspect, const Entity& entity)
	{
		Archetype* archetype = getOrCreateArchetype(aspect);

		const bool wasEmpty = archetype->isEmpty();
		archetype->addEntity(entity);
		if (wasEmpty)
		{
			m_versionManager->makeDirty(archetype->aspect());
		}

		m_entityToArchetype[entity] = archetype;
	}

	const Archetype& ArchetypeManager::getEntityArchetype(Entity entity) const
	{
		SASSERT(isEntityTracked(entity))
		return *m_entityToArchetype.at(entity);
	}

	const Aspect& ArchetypeManager::getEntityAspect(Entity entity) const
	{
		return getEntityArchetype(entity).aspect();
	}

	heap_vector<Archetype*> ArchetypeManager::queryArchetypes(const Aspect& includeAspect,
	                                                          const Aspect& excludeAspect) const
	{
		auto result = makeHeapVector<Archetype*>(m_allocator);
		auto* rootAspect = m_aspectRegistry->getAspect(includeAspect);
		if (!rootAspect)
		{
			return result;
		}

		auto descendants = m_aspectRegistry->getDescendantAspects(includeAspect);
		descendants.push_back(rootAspect);

		for (auto* aspect : descendants)
		{
			if (!aspect->intersects(excludeAspect))
			{
				auto* archetype = findArchetype(*aspect);
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
		auto result = makeHeapVector<Archetype*>(m_allocator);
		auto* rootAspect = m_aspectRegistry->getAspect(includeAspect);
		if (!rootAspect)
		{
			return result;
		}

		auto allocMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto descendants = m_aspectRegistry->getDescendantAspects(includeAspect);
		descendants.push_back(rootAspect);

		for (auto* aspect : descendants)
		{
			if (!aspect->intersects(excludeAspect))
			{
				auto* archetype = findArchetype(*aspect);
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

	Archetype& ArchetypeManager::getEntityArchetypeInternal(Entity entity)
	{
		SASSERT(isEntityTracked(entity))
		return *m_entityToArchetype.at(entity);
	}

	bool ArchetypeManager::isEntityTracked(Entity entity) const
	{
		return m_entityToArchetype.find(entity) != m_entityToArchetype.end();
	}

	void ArchetypeManager::moveEntitiesBetweenArchetypes(Archetype* from,
	                                                     Archetype* to,
	                                                     eastl::span<const Entity> entities)
	{
		const bool fromWasEmpty = from->isEmpty();
		const bool toWasEmpty = to->isEmpty();

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto newLocations = to->addEntities(entities);
		auto newLocationMap = makeScratchMap<Entity, eastl::pair<Chunk*, sizet>, Entity::hash>(
			FrameScratchAllocator::get());
		for (sizet i = 0; i < entities.size(); ++i)
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
			                         from,
			                         to);
		}

		from->removeEntities(entities, &to->aspect());

		for (auto entity : entities)
		{
			m_entityToArchetype[entity] = to;
		}

		const bool fromIsNowEmpty = from->isEmpty();
		const bool toIsNowEmpty = to->isEmpty();

		if (!fromWasEmpty && fromIsNowEmpty)
		{
			m_versionManager->makeDirty(from->aspect());
		}
		if (toWasEmpty && !toIsNowEmpty)
		{
			m_versionManager->makeDirty(to->aspect());
		}
	}

	void ArchetypeManager::copyCompatibleComponents(Chunk* fromChunk,
	                                                sizet fromIndex,
	                                                Chunk* toChunk,
	                                                sizet toIndex,
	                                                const Archetype* fromArchetype,
	                                                const Archetype* toArchetype) const
	{
		const auto& commonIds = fromArchetype->aspect().getIntersection(toArchetype->aspect());

		for (const auto& componentId : commonIds)
		{
			const auto& metadata = m_metadataRegistry->getMetadata(componentId);

			const int fromComponentIndex = fromArchetype->getComponentIndex(componentId);
			const int toComponentIndex = toArchetype->getComponentIndex(componentId);

			void* srcPtr = fromChunk->getComponentDataPtrByIndex(fromComponentIndex, fromIndex);
			void* dstPtr = toChunk->getComponentDataPtrByIndex(toComponentIndex, toIndex);

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
