#pragma once
#include <bitset>

#include <EASTL/span.h>

#include "Entity.hpp"

#include "base/CollectionAliases.hpp"
#include "base/memory/HeapAllocator.hpp"

#include "ecs/Aspect.hpp"

namespace spite
{
	class ComponentMetadataRegistry;
	constexpr sizet DEFAULT_CHUNK_CAPACITY = 64;
	constexpr sizet DEFAULT_COMPONENTS_INLINE_CAPACITY = 16;


	class Chunk
	{
		static constexpr sizet CAPACITY = DEFAULT_CHUNK_CAPACITY;

	private:
		Aspect m_aspect;
		sizet m_count;
		const ComponentMetadataRegistry* m_metadataRegistry;
		HeapAllocator& m_allocator;

		// A single block of memory for all component arrays.
		std::byte* m_storageBlock;

		eastl::array<Entity, CAPACITY> m_entities;
		heap_sbo_vector<std::byte*, DEFAULT_COMPONENTS_INLINE_CAPACITY> m_componentDataStarts;

		// Per-component modification tracking bitsets.
		heap_sbo_vector<std::bitset<CAPACITY>, DEFAULT_COMPONENTS_INLINE_CAPACITY>
		m_modifiedBitsets;
		heap_sbo_vector<std::bitset<CAPACITY>, DEFAULT_COMPONENTS_INLINE_CAPACITY> m_enabledBitsets;

	public:
		Chunk(Aspect aspect,
		      const ComponentMetadataRegistry* metadataRegistry,
		      HeapAllocator& allocator);

		~Chunk();

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;

		Chunk(Chunk&& other) noexcept;

		Chunk& operator=(Chunk&& other) noexcept;

		[[nodiscard]] bool full() const;

		[[nodiscard]] bool empty() const;

		[[nodiscard]] sizet count() const;

		[[nodiscard]] sizet capacity() const;

		[[nodiscard]] const Aspect& aspect() const;

		// Adds an entity ID and returns the index within the chunk where its components will be stored.
		// The caller is responsible for constructing/placing the component data.
		sizet addEntity(const Entity entity);

		// Removes an entity at a given index within this chunk.
		// Uses swap-and-pop: the last entity in the chunk takes the place of the removed one.
		// Returns the Entity ID that was moved into the `entityChunkIndex` slot (or Entity::undefined if last was removed).
		// Destructors for removedEntity should be called in Archetype
		Entity removeEntityAndSwap(const sizet entityChunkIndex);


		// marks component as modified
		template <typename TComponent>
		TComponent& component(const sizet entityChunkIdx);

		// does not mark component as modified
		template <typename TComponent>
		const TComponent& component(const sizet entityChunkIndex) const;

		// Get a raw pointer to the array of components of a specific type.
		// The array contains `m_count` valid components.
		template <typename TComponent>
		TComponent* componentArray();
		template <typename TComponent>
		const TComponent* componentArray() const;

		[[nodiscard]] Entity entity(const sizet entityChunkIndex) const;

		[[nodiscard]] eastl::span<const Entity> entities() const;

		template <typename TComponent>
		[[nodiscard]] bool wasModifiedLastFrame(const sizet entityChunkIndex) const;

		[[nodiscard]] bool wasModifiedLastFrame(const sizet entityChunkIndex,
		                                        const std::type_index targetType) const;

		[[nodiscard]] bool wasModifiedLastFrameByIndex(const sizet componentIndexInChunk,
		                                               const sizet entityChunkIndex) const;

		const std::bitset<CAPACITY>* getModifiedBitset(std::type_index type) const;

		void resetModificationTracking();

		// Get a raw uncasted ptr to the component data for a specific entity and component type.
		// Marks component as modified
		[[nodiscard]] void* componentDataPtr(const sizet entityChunkIdx,
		                                     const std::type_index targetType);

		// Get a raw uncasted ptr to the component data for a specific entity and component type.
		// Does not mark component as modified
		[[nodiscard]] const void* componentDataPtr(const sizet entityChunkIdx,
		                                           const std::type_index targetType) const;

		// Gets a raw pointer to an entity's component data using a pre-calculated index. (O(1) access)
		void* getComponentDataPtrByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);

		const void* getComponentDataPtrByIndex(sizet componentIndexInChunk,
		                                       sizet entityIndexInChunk) const;

		// Marks a component as modified using a pre-calculated index. (O(1) access)
		void markModifiedByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);

		void enableComponentByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);
		void disableComponentByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk);

		template <typename TComponent>
		void enableComponent(sizet entityChunkIndex);

		template <typename TComponent>
		void disableComponent(sizet entityChunkIndex);

		[[nodiscard]] bool isComponentEnabledByIndex(sizet componentIndexInChunk,
		                                             sizet entityIndexInChunk) const;

		template <typename TComponent>
		[[nodiscard]] bool isComponentEnabled(sizet entityChunkIndex) const;
	};

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
		SASSERTM(false, "Component type %s is not in chunk's aspect\n")
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
		SASSERTM(false, "Component type %s is not in chunk's aspect\n")
		return nullptr;
	}

	template <typename TComponent>
	bool Chunk::wasModifiedLastFrame(const sizet entityChunkIndex) const
	{
		return wasModifiedLastFrame(entityChunkIndex, typeid(TComponent));
	}

	inline bool Chunk::wasModifiedLastFrameByIndex(const sizet componentIndexInChunk,
	                                               const sizet entityChunkIndex) const
	{
		return m_modifiedBitsets[componentIndexInChunk].test(entityChunkIndex);
	}

	inline void Chunk::enableComponentByIndex(sizet componentIndexInChunk, sizet entityIndexInChunk)
	{
		m_enabledBitsets[componentIndexInChunk].set(entityIndexInChunk);
	}

	inline void Chunk::disableComponentByIndex(sizet componentIndexInChunk,
	                                           sizet entityIndexInChunk)
	{
		m_enabledBitsets[componentIndexInChunk].reset(entityIndexInChunk);
	}

	template <typename TComponent>
	void Chunk::enableComponent(sizet entityChunkIndex)
	{
		const auto& componentTypes = m_aspect.getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == typeid(TComponent))
			{
				enableComponentByIndex(i, entityChunkIndex);
				return;
			}
		}
	}

	template <typename TComponent>
	void Chunk::disableComponent(sizet entityChunkIndex)
	{
		const auto& componentTypes = m_aspect.getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == typeid(TComponent))
			{
				disableComponentByIndex(i, entityChunkIndex);
				return;
			}
		}
	}

	inline bool Chunk::isComponentEnabledByIndex(sizet componentIndexInChunk,
	                                             sizet entityIndexInChunk) const
	{
		return m_enabledBitsets[componentIndexInChunk].test(entityIndexInChunk);
	}

	template <typename TComponent>
	bool Chunk::isComponentEnabled(sizet entityChunkIndex) const
	{
		const auto& componentTypes = m_aspect.getTypes();
		for (size_t i = 0; i < componentTypes.size(); ++i)
		{
			if (componentTypes[i] == typeid(TComponent))
			{
				return isComponentEnabledByIndex(i, entityChunkIndex);
			}
		}
		return false;
	}
}
