#include "Chunk.hpp"

#include "ComponentMetadata.hpp"
#include "Entity.hpp"

namespace spite
{
	Chunk::Chunk(const Aspect* aspect,
		sizet capacity,
		const ComponentMetadataRegistry* metadataRegistry,
		const HeapAllocator& allocator): m_aspect(aspect), m_entities(allocator),
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

	Chunk::~Chunk()
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

	bool Chunk::isFull() const
	{ return m_count >= m_capacity; }

	bool Chunk::isEmpty() const
	{ return m_count == 0; }

	sizet Chunk::getCount() const
	{ return m_count; }

	sizet Chunk::getCapacity() const
	{ return m_capacity; }

	const Aspect& Chunk::getAspect() const
	{ return *m_aspect; }

	sizet Chunk::addEntity(const Entity entity)
	{
		SASSERT(!isFull())
		m_entities.push_back(entity);
		// Component data slots are implicitly reserved.
		return m_count++;
	}

	Entity Chunk::removeEntityAndSwap(const sizet entityChunkIndex)
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

	void* Chunk::getComponentDataPtr(const sizet entityChunkIdx, const std::type_index targetType)
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


	Entity Chunk::getEntity(const sizet entityChunkIndex) const
	{
		SASSERT(entityChunkIndex < m_count)
		return m_entities[entityChunkIndex];
	}

	const heap_vector<Entity>& Chunk::getEntities() const
	{
		return m_entities;
	}
}
