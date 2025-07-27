#include "Archetype.hpp"

#include "base/CollectionUtilities.hpp"

#include "ecs/core/ComponentMetadataRegistry.hpp"

namespace spite
{
	Archetype::Archetype(const Aspect* aspect,
	                     HeapAllocator& allocator): m_aspect(aspect),
	                                                m_componentIdToIndexMap(
		                                                makeHeapMap<
			                                                ComponentID, int>(allocator)),
	                                                m_chunks(makeHeapVector<Chunk*>(allocator)),
	                                                m_freeChunks(
		                                                makeHeapVector<Chunk*>(
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

	Archetype::~Archetype()
	{
		for(Chunk* chunk : m_chunks)
		{
			m_allocator.delete_object(chunk);
		}
		for(Chunk* chunk : m_freeChunks)
		{
			m_allocator.delete_object(chunk);
		}
	}

	eastl::pair<Chunk*, sizet> Archetype::addEntity(const Entity entity)
	{
		Chunk* targetChunk = nullptr;
		sizet chunkIndex;

		// Start search from the last known non-full chunk.
		if (m_firstNonFullChunkIdx < m_chunks.size() && !m_chunks[m_firstNonFullChunkIdx]->full())
		{
			targetChunk = m_chunks[m_firstNonFullChunkIdx];
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
					targetChunk = m_chunks[chunkIndex];
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
				m_chunks.push_back(m_freeChunks.back());
				m_freeChunks.pop_back();
				chunkIndex = m_chunks.size() - 1;
				targetChunk = m_chunks[chunkIndex];
				m_firstNonFullChunkIdx = chunkIndex;
			}
			else
			{
				Chunk* newChunk = m_allocator.new_object<Chunk>(m_aspect, m_allocator);
				targetChunk = newChunk;
				m_chunks.push_back(newChunk);
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
			Chunk* chunk = m_chunks[chunkIdx];
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
				Chunk* newChunk = m_allocator.new_object<Chunk>(m_aspect, m_allocator);
				m_chunks.push_back(newChunk);
			}

			for (sizet chunkIdx = m_chunks.size() - numNewChunks; chunkIdx < m_chunks.size() &&
			     entitiesAdded < totalToAdd; ++chunkIdx)
			{
				Chunk* chunk = m_chunks[chunkIdx];
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

	void Archetype::removeEntity(Entity entity, const DestructionContext& context)
	{
		auto it = m_entityLocations.find(entity);
		if (it == m_entityLocations.end())
		{
			SASSERTM(false, "Entity %llu not in this archetype for removal\n", entity.id())
			return;
		}

		sizet chunkIdx = it->second.first;
		sizet entityIdxInChunk = it->second.second;

		Chunk* chunk = m_chunks[chunkIdx];

		// Before swapping, call destruction policies for components of the entity being removed
		for (const auto& componentId : m_aspect->getComponentIds())
		{
			const auto& meta = ComponentMetadataRegistry::getMetadata(componentId);
			void* componentPtr = chunk->getComponentDataPtrByIndex(
				getComponentIndex(componentId),
				entityIdxInChunk);
			meta.destructionPolicy(componentPtr, context);
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
			m_freeChunks.push_back(m_chunks[chunkIdx]);

			// If the empty chunk was not the last one, swap the last chunk into its place
			// to keep the active chunk vector contiguous.
			if (chunkIdx != lastChunkIdx)
			{
				// Move the last chunk into the now-empty slot
				m_chunks[chunkIdx] = m_chunks[lastChunkIdx];

				// Update the locations for all entities in the chunk that was just moved.
				Chunk* movedChunk = m_chunks[chunkIdx];
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

	void Archetype::removeEntities(eastl::span<const Entity> entities, const DestructionContext& context,
	                               const Aspect* skipDestructionAspect)
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
				Chunk* chunk = m_chunks[it->second.first];
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

			for (sizet indexToRemove : indices)
			{
				// Call destruction policies
				for (const auto& componentId : m_aspect->getComponentIds())
				{
					if (skipDestructionAspect && skipDestructionAspect->contains(componentId)) continue;

					const auto& meta = ComponentMetadataRegistry::getMetadata(componentId);
					void* componentPtr = chunk->getComponentDataPtrByIndex(
						getComponentIndex(componentId),
						indexToRemove);
					meta.destructionPolicy(componentPtr, context);
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

	const heap_vector<Chunk*>& Archetype::getChunks() const
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
			return {m_chunks[it->second.first], it->second.second};
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

	void Archetype::destroyAllComponentsInChunk(Chunk* chunk, const DestructionContext& destructionContext) const
	{
		const auto& componentIds = m_aspect->getComponentIds();
		for (sizet entityIdx = 0; entityIdx < chunk->count(); ++entityIdx)
		{
			for (size_t i = 0; i < componentIds.size(); ++i)
			{
				const auto& meta = ComponentMetadataRegistry::getMetadata(componentIds[i]);
				auto componentPtr = chunk->getComponentDataPtrByIndex(i, entityIdx);
				meta.destructionPolicy(componentPtr, destructionContext);
			}
		}
	}

	void Archetype::destroyAllComponents(const DestructionContext& destructionContext) const
	{
		for (auto& chunk : m_chunks)
		{
			destroyAllComponentsInChunk(chunk, destructionContext);
		}
	}
}
