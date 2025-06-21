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
}
