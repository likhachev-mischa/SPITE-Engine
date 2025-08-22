#include "Chunk.hpp"

#include <algorithm>

#include "ecs/core/ComponentMetadataRegistry.hpp"
#include "ecs/core/Entity.hpp"

#include "base/CollectionUtilities.hpp"

namespace spite
{
	Chunk::Chunk(const Aspect* aspect,
	             HeapAllocator& allocator): m_aspect(aspect), m_count(0),
	                                        m_allocator(allocator), m_storageBlock(nullptr),
	                                        m_componentDataStarts(
		                                        makeSboVector<
			                                        std::byte*, DEFAULT_COMPONENTS_INLINE_CAPACITY>(
			                                        m_allocator)),
	                                        m_modifiedBitsets(
		                                        makeSboVector<
			                                        std::bitset<CAPACITY>,
			                                        DEFAULT_COMPONENTS_INLINE_CAPACITY>(
			                                        m_allocator)),
	                                        m_enabledBitsets(
		                                        makeSboVector<
			                                        std::bitset<CAPACITY>,
			                                        DEFAULT_COMPONENTS_INLINE_CAPACITY>(
			                                        m_allocator))
	{
		const auto& componentIds = m_aspect->getComponentIds();
		const auto numComponentTypes = componentIds.size();
		m_componentDataStarts.resize(numComponentTypes);
		m_modifiedBitsets.resize(numComponentTypes);
		m_enabledBitsets.resize(numComponentTypes);

		// Calculate total size, offsets, and max alignment
		sizet totalSize = 0;
		sizet maxAlignment = alignof(std::max_align_t);
		sbo_vector<sizet> offsets;
		offsets.resize(numComponentTypes);
		for (sizet i = 0; i < componentIds.size(); ++i)
		{
			const auto& meta = ComponentMetadataRegistry::getMetadata(componentIds[i]);
			maxAlignment = std::max(meta.alignment, maxAlignment);
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
		for (sizet i = 0; i < componentIds.size(); ++i)
		{
			m_componentDataStarts[i] = m_storageBlock + offsets[i];
		}

		for (auto& enabledBitset : m_enabledBitsets)
		{
			enabledBitset.set(); // Enable all by default
		}
	}

	Chunk::~Chunk()
	{
		m_allocator.deallocate(m_storageBlock, 0);
	}

	Chunk::Chunk(Chunk&& other) noexcept: m_aspect(other.m_aspect),
	                                      m_count(other.m_count),
	                                      m_allocator(other.m_allocator),
	                                      m_storageBlock(other.m_storageBlock),
	                                      m_entities(other.m_entities),
	                                      m_componentDataStarts(
		                                      std::move(other.m_componentDataStarts)),
	                                      m_modifiedBitsets(std::move(other.m_modifiedBitsets)),
	                                      m_enabledBitsets(std::move(other.m_enabledBitsets))
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

			m_aspect = other.m_aspect;
			m_count = other.m_count;
			m_allocator = other.m_allocator;
			m_storageBlock = other.m_storageBlock;
			m_entities = other.m_entities;
			m_componentDataStarts = std::move(other.m_componentDataStarts);
			m_modifiedBitsets = std::move(other.m_modifiedBitsets);
			m_enabledBitsets = std::move(other.m_enabledBitsets);

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

	sizet Chunk::size() const
	{
		return m_count;
	}

	sizet Chunk::capacity() const
	{
		return CAPACITY;
	}

	const Aspect& Chunk::aspect() const
	{
		return *m_aspect;
	}

	sizet Chunk::addEntity(const Entity entity)
	{
		SASSERT(!full())

		const sizet newEntityIndex = m_count;
		m_entities[newEntityIndex] = entity;
		// A new entity's components are considered modified for the current frame.
		const sizet numComponentTypes = m_aspect->getComponentIds().size();
		for (sizet i = 0; i < numComponentTypes; ++i)
		{
			m_modifiedBitsets[i].set(newEntityIndex);
			m_enabledBitsets[i].set(newEntityIndex);
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

			const auto& componentIds = m_aspect->getComponentIds();
			for (size_t i = 0; i < componentIds.size(); ++i)
			{
				const auto& meta = ComponentMetadataRegistry::getMetadata(componentIds[i]);
				const sizet componentSize = meta.size;
				std::byte* componentArray = m_componentDataStarts[i];

				std::byte* dest = componentArray + (entityChunkIndex * componentSize);
				std::byte* src = componentArray + (lastEntityIndex * componentSize);

				meta.moveAndDestroy(dest, src);

				// The modification and enabled statuses must also be swapped.
				const bool wasLastModified = m_modifiedBitsets[i].test(lastEntityIndex);
				m_modifiedBitsets[i].set(entityChunkIndex, wasLastModified);
				const bool wasLastEnabled = m_enabledBitsets[i].test(lastEntityIndex);
				m_enabledBitsets[i].set(entityChunkIndex, wasLastEnabled);
			}
		}
		m_count--;
		return swappedEntity;
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

	void Chunk::resetModificationTracking()
	{
		for (auto& bitset : m_modifiedBitsets)
		{
			bitset.reset();
		}
	}

	void* Chunk::getComponentDataPtrByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk)
	{
		SASSERT(componentIndexInChunk < m_aspect->getComponentIds().size())
		SASSERT(entityIndexInChunk < m_count)
		markModifiedByIndex(componentIndexInChunk, entityIndexInChunk);
		const auto& meta = ComponentMetadataRegistry::getMetadata(
			m_aspect->getComponentIds()[componentIndexInChunk]);
		std::byte* componentArrayStart = m_componentDataStarts[componentIndexInChunk];
		return componentArrayStart + (entityIndexInChunk * meta.size);
	}

	const void* Chunk::getComponentDataPtrByIndex(sizet componentIndexInChunk,
	                                              sizet entityIndexInChunk) const
	{
		SASSERT(componentIndexInChunk < m_aspect->getComponentIds().size())
		SASSERT(entityIndexInChunk < m_count)
		const auto& meta = ComponentMetadataRegistry::getMetadata(
			m_aspect->getComponentIds()[componentIndexInChunk]);
		const std::byte* componentArrayStart = m_componentDataStarts[componentIndexInChunk];
		return componentArrayStart + (entityIndexInChunk * meta.size);
	}

	void Chunk::markModifiedByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk)
	{
		SASSERT(componentIndexInChunk < m_aspect->getComponentIds().size())
		SASSERT(entityIndexInChunk < m_count)
		m_modifiedBitsets[componentIndexInChunk].set(entityIndexInChunk);
	}

	bool Chunk::wasModifiedLastFrameByIndex(const sizet componentIndexInChunk,
	                                        const sizet entityIndexInChunk) const
	{
		return m_modifiedBitsets[componentIndexInChunk].test(entityIndexInChunk);
	}

	void Chunk::enableComponentByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk)
	{
		m_enabledBitsets[componentIndexInChunk].set(entityIndexInChunk);
	}

	void Chunk::disableComponentByIndex(sizet componentIndexInChunk,
	                                           sizet entityIndexInChunk)
	{
		m_enabledBitsets[componentIndexInChunk].reset(entityIndexInChunk);
	}

	bool Chunk::isComponentEnabledByIndex(sizet componentIndexInChunk,
	                                             sizet entityIndexInChunk) const
	{
		return m_enabledBitsets[componentIndexInChunk].test(entityIndexInChunk);
	}
}
