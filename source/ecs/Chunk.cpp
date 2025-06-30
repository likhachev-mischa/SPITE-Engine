#include "Chunk.hpp"

#include "ComponentMetadata.hpp"
#include "Entity.hpp"

namespace spite
{
	Chunk::Chunk(Aspect aspect,
		const ComponentMetadataRegistry* metadataRegistry,
		HeapAllocator& allocator): m_aspect(std::move(aspect)), m_count(0),
		                           m_metadataRegistry(metadataRegistry),
		                           m_allocator(allocator), m_storageBlock(nullptr)
	{
		const auto& componentTypes = m_aspect.getTypes();
		SASSERT(componentTypes.size() <= MAX_COMPONENT_TYPES);

		// Calculate total size, offsets, and max alignment
		sizet totalSize = 0;
		sizet maxAlignment = 1; // Or a sensible default like alignof(std::max_align_t)
		eastl::array<sizet, MAX_COMPONENT_TYPES> offsets{};
		for (sizet i = 0; i < componentTypes.size(); ++i)
		{
			const auto& meta = m_metadataRegistry->getMetadata(componentTypes[i]);
			if (meta.alignment > maxAlignment)
			{
				maxAlignment = meta.alignment;
			}
			// Align the current offset
			if (totalSize % meta.alignment != 0)
			{
				totalSize += meta.alignment - (totalSize % meta.alignment);
			}
			offsets[i] = totalSize;
			totalSize += meta.size * CAPACITY;
		}

		// Perform the single allocation
		m_storageBlock = static_cast<std::byte*>(m_allocator.allocate(totalSize, maxAlignment));

		// Set component start pointers
		for (sizet i = 0; i < componentTypes.size(); ++i)
		{
			m_componentDataStarts[i] = m_storageBlock + offsets[i];
		}
	}

	Chunk::~Chunk()
	{
		if (m_storageBlock)
		{
			// Call destructors for all non-trivial components before freeing the memory block.
			const auto& componentTypes = m_aspect.getTypes();
			for (sizet entityIdx = 0; entityIdx < m_count; ++entityIdx)
			{
				for (size_t i = 0; i < componentTypes.size(); ++i)
				{
					const auto& meta = m_metadataRegistry->getMetadata(componentTypes[i]);
					if (!meta.isTriviallyRelocatable)
					{
						const sizet componentSize = meta.size;
						std::byte* componentArray = m_componentDataStarts[i];
						std::byte* componentPtr = componentArray + (entityIdx * componentSize);
						meta.destructor(componentPtr);
					}
				}
			}
			m_allocator.deallocate(m_storageBlock, 0);
		}
	}

	Chunk::Chunk(Chunk&& other) noexcept: m_aspect(std::move(other.m_aspect)),
	                                      m_count(other.m_count),
	                                      m_metadataRegistry(other.m_metadataRegistry),
	                                      m_allocator(other.m_allocator),
	                                      m_storageBlock(other.m_storageBlock),
	                                      m_entities(other.m_entities),
	                                      m_componentDataStarts(other.m_componentDataStarts),
	                                      m_modifiedBitsets(other.m_modifiedBitsets)
	{
		other.m_storageBlock = nullptr;
		other.m_count = 0;
	}

	Chunk& Chunk::operator=(Chunk&& other) noexcept
	{
		if (this != &other)
		{
			if (m_storageBlock)
			{
				m_allocator.deallocate(m_storageBlock, 0);
			}

			m_aspect = std::move(other.m_aspect);
			m_count = other.m_count;
			m_metadataRegistry = other.m_metadataRegistry;
			m_allocator = other.m_allocator;
			m_storageBlock = other.m_storageBlock;
			m_entities = other.m_entities;
			m_componentDataStarts = other.m_componentDataStarts;
			m_modifiedBitsets = other.m_modifiedBitsets;

			other.m_storageBlock = nullptr;
			other.m_count = 0;
		}
		return *this;
	}

	bool Chunk::full() const
	{
		return m_count >= CAPACITY;
	}

	bool Chunk::empty() const
	{
		return m_count == 0;
	}

	sizet Chunk::count() const
	{
		return m_count;
	}

	sizet Chunk::capacity() const
	{
		return CAPACITY;
	}

	const Aspect& Chunk::aspect() const
	{
		return m_aspect;
	}

	sizet Chunk::addEntity(const Entity entity)
	{
		SASSERT(!full())

		const sizet newEntityIndex = m_count;
		m_entities[newEntityIndex] = entity;
		// A new entity's components are considered modified for the current frame.
		const sizet numComponentTypes = m_aspect.getTypes().size();
		for (sizet i = 0; i < numComponentTypes; ++i)
		{
			m_modifiedBitsets[i].set(newEntityIndex);
		}

		return m_count++;
	}

	Entity Chunk::removeEntityAndSwap(const sizet entityChunkIndex)
	{
		SASSERT(entityChunkIndex < m_count)

		Entity swappedEntity = Entity::undefined();
		const sizet lastEntityIndex = m_count - 1;

		if (entityChunkIndex != lastEntityIndex)
		{
			m_entities[entityChunkIndex] = m_entities[lastEntityIndex];
			swappedEntity = m_entities[entityChunkIndex];

			const auto& componentTypes = m_aspect.getTypes();
			for (size_t i = 0; i < componentTypes.size(); ++i)
			{
				const auto& meta = m_metadataRegistry->getMetadata(componentTypes[i]);
				const sizet componentSize = meta.size;
				std::byte* componentArray = m_componentDataStarts[i];

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

				// The modification status must also be swapped.
				const bool wasLastModified = m_modifiedBitsets[i].test(lastEntityIndex);
				m_modifiedBitsets[i].set(entityChunkIndex, wasLastModified);
			}
		}
		m_count--;
		return swappedEntity;
	}

	template <typename TComponent>
	TComponent& Chunk::component(const sizet entityChunkIdx)
	{
		std::type_index targetType = typeid(TComponent);
		return static_cast<TComponent*>(componentDataPtr(entityChunkIdx, targetType));
	}

	template <typename TComponent>
	const TComponent& Chunk::component(const sizet entityChunkIndex) const
	{
		std::type_index targetType = typeid(TComponent);
		return static_cast<const TComponent*>(componentDataPtr(entityChunkIndex, targetType));
	}

	template <typename TComponent>
	TComponent* Chunk::componentArray()
	{
		std::type_index targetType = typeid(TComponent);
		const auto& componentTypes = m_aspect.getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == targetType)
			{
				for (sizet entityIdx = 0; entityIdx < m_count; ++entityIdx)
				{
					m_modifiedBitsets[i].set(entityIdx);
				}
				return reinterpret_cast<TComponent*>(m_componentDataStarts[i]);
			}
		}
		SASSERTM(false, "Component type %s is not in chunk's aspect\n", targetType.name())
		return nullptr;
	}

	template <typename TComponent>
	const TComponent* Chunk::componentArray() const
	{
		const std::type_index targetType = typeid(TComponent);
		const auto& componentTypes = m_aspect.getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == targetType)
			{
				return reinterpret_cast<const TComponent*>(m_componentDataStarts[i]);
			}
		}
		SASSERTM(false, "Component type %s is not in chunk's aspect\n", targetType.name())
		return nullptr;
	}

	Entity Chunk::entity(const sizet entityChunkIndex) const
	{
		SASSERT(entityChunkIndex < m_count)
		return m_entities[entityChunkIndex];
	}

	eastl::span<const Entity> Chunk::entities() const
	{
		return {m_entities.data(), m_count};
	}

	template <typename TComponent>
	bool Chunk::wasModifiedLastFrame(const sizet entityChunkIndex) const
	{
		return wasModifiedLastFrame(entityChunkIndex, typeid(TComponent));
	}

	bool Chunk::wasModifiedLastFrame(const sizet entityChunkIndex,
		const std::type_index targetType) const
	{
		SASSERT(entityChunkIndex < m_count)
		const auto& componentTypes = m_aspect.getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == targetType)
			{
				return m_modifiedBitsets[i].test(entityChunkIndex);
			}
		}
		return false;
	}

	const std::bitset<Chunk::CAPACITY>* Chunk::getModifiedBitset(std::type_index type) const
	{
		const auto& componentTypes = m_aspect.getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == type)
			{
				return &m_modifiedBitsets[i];
			}
		}
		return nullptr;
	}

	void Chunk::resetModificationTracking()
	{
		for (auto& bitset : m_modifiedBitsets)
		{
			bitset.reset();
		}
	}

	void* Chunk::componentDataPtr(const sizet entityChunkIdx, const std::type_index targetType)
	{
		SASSERT(entityChunkIdx < m_count)

		const auto& componentTypes = m_aspect.getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == targetType)
			{
				m_modifiedBitsets[i].set(entityChunkIdx);
				const auto& meta = m_metadataRegistry->getMetadata(targetType);
				std::byte* componentArrayStart = m_componentDataStarts[i];
				return componentArrayStart + (entityChunkIdx * meta.size);
			}
		}
		SASSERTM(false, "Component type %s is not in chunk's aspect\n", targetType.name())
		return nullptr;
	}

	const void* Chunk::componentDataPtr(const sizet entityChunkIdx,
		const std::type_index targetType) const
	{
		SASSERT(entityChunkIdx < m_count)

		const auto& componentTypes = m_aspect.getTypes();
		for (sizet i = 0; i < componentTypes.size(); ++i)
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

	int Chunk::getComponentIndex(const std::type_index type) const
	{
		const auto& componentTypes = m_aspect.getTypes();
		for (sizet i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == type)
			{
				return static_cast<int>(i);
			}
		}
		return -1;
	}

	void* Chunk::getComponentDataPtrByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk)
	{
		SASSERT(componentIndexInChunk < m_aspect.getTypes().size())
		SASSERT(entityIndexInChunk < m_count)
		const auto& meta = m_metadataRegistry->getMetadata(m_aspect.getTypes()[componentIndexInChunk]);
		std::byte* componentArrayStart = m_componentDataStarts[componentIndexInChunk];
		return componentArrayStart + (entityIndexInChunk * meta.size);
	}

	const void* Chunk::getComponentDataPtrByIndex(sizet componentIndexInChunk,
		sizet entityIndexInChunk) const
	{
		SASSERT(componentIndexInChunk < m_aspect.getTypes().size())
		SASSERT(entityIndexInChunk < m_count)
		const auto& meta = m_metadataRegistry->getMetadata(m_aspect.getTypes()[componentIndexInChunk]);
		const std::byte* componentArrayStart = m_componentDataStarts[componentIndexInChunk];
		return componentArrayStart + (entityIndexInChunk * meta.size);
	}

	void Chunk::markModifiedByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk)
	{
		SASSERT(componentIndexInChunk < m_aspect.getTypes().size())
		SASSERT(entityIndexInChunk < m_count)
		m_modifiedBitsets[componentIndexInChunk].set(entityIndexInChunk);
	}
}
