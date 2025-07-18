#pragma once

#include <EASTL/vector.h>
#include <EASTL/unordered_set.h>

#include "base/CollectionAliases.hpp"
#include "base/CollectionUtilities.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "ecs/core/ComponentMetadata.hpp"
#include "ecs/core/ComponentMetadataRegistry.hpp"
#include "ecs/core/IComponent.hpp"

namespace spite
{
	struct alignas(8) SharedComponentHandle
	{
		ComponentID componentId = INVALID_COMPONENT_ID;
		u32 dataIndex = -1;

		SharedComponentHandle() = default;

		SharedComponentHandle(ComponentID componentId, u32 dataIndex);

		bool operator==(const SharedComponentHandle&) const = default;
		bool operator!=(const SharedComponentHandle&) const = default;
	};


	struct ISharedComponentPool
	{
		virtual ~ISharedComponentPool() = default;

		ISharedComponentPool() = default;
		ISharedComponentPool(const ISharedComponentPool& other) = delete;
		ISharedComponentPool(ISharedComponentPool&& other) noexcept = delete;
		ISharedComponentPool& operator=(const ISharedComponentPool& other) = delete;
		ISharedComponentPool& operator=(ISharedComponentPool&& other) noexcept = delete;

		virtual void incrementRef(u32 index) = 0;
		virtual void decrementRef(u32 index) = 0;
		virtual void destroy(HeapAllocator& allocator) = 0;
	};

	template <t_shared_component T>
	class TypedSharedComponentPool final : public ISharedComponentPool
	{
		struct DataHasher
		{
			using is_transparent = void;
			const TypedSharedComponentPool* m_pool;
			sizet operator()(u32 index) const;
			sizet operator()(const T& item) const;
		};

		struct DataEqual
		{
			using is_transparent = void;
			const TypedSharedComponentPool* m_pool;

			bool operator()(u32 indexA, u32 indexB) const;

			bool operator()(u32 indexA, const T& itemB) const;

			bool operator()(const T& itemA, u32 indexB) const;
		};

		heap_vector<T> m_data;
		heap_vector<u32> m_refCounts;
		heap_vector<u32> m_freeList;
		eastl::unordered_set<u32, DataHasher, DataEqual, HeapAllocatorAdapter<u32>> m_valueToIndexSet;

	private:
		u32 createUniqueInstance(const T& item);

	public:
		TypedSharedComponentPool(const HeapAllocator& allocator)
			: m_data(makeHeapVector<T>(allocator))
			  , m_refCounts(makeHeapVector<u32>(allocator))
			  , m_freeList(makeHeapVector<u32>(allocator))
			  , m_valueToIndexSet(10, DataHasher{this}, DataEqual{this}, HeapAllocatorAdapter<u32>(allocator))
		{
		}

		u32 findOrCreateIndex(const T& item)
		{
			auto it = m_valueToIndexSet.find_as(item, 
				        m_valueToIndexSet.hash_function(), 
				        m_valueToIndexSet.key_eq());

			if (it != m_valueToIndexSet.end())
			{
				return *it;
			}

			u32 newIndex = createUniqueInstance(item);
			m_valueToIndexSet.insert(newIndex);
			return newIndex;
		}

		const T& getData(u32 index) const
		{
			SASSERT(index < m_data.size())
			return m_data[index];
		}

		// Copy-on-write logic
		u32 getMutableDataIndex(u32 index)
		{
			SASSERT(index < m_data.size())
			if (m_refCounts[index] > 1)
			{
				// If the component is shared, create a copy
				T copy = m_data[index];
				// Decrement the ref count of the old component
				decrementRef(index);
				// Create a new, unique component and return its index
				u32 newIndex = createUniqueInstance(copy);
				incrementRef(newIndex); // It starts with a ref count of 1
				return newIndex;
			}
			// If the component is not shared (or has only one owner), we can modify it in place,
			// but we must remove it from the interning map to prevent future sharing of its modified state.
			m_valueToIndexSet.erase(index);
			return index;
		}

		T& getDataByIndex(u32 index)
		{
			SASSERT(index < m_data.size())
			return m_data[index];
		}

		void incrementRef(u32 index) override
		{
			SASSERT(index < m_refCounts.size())
			m_refCounts[index]++;
		}

		void decrementRef(u32 index) override
		{
			SASSERT(index < m_refCounts.size())
			if (m_refCounts[index] > 0)
			{
				m_refCounts[index]--;
				if (m_refCounts[index] == 0)
				{
					m_valueToIndexSet.erase(index);
					m_data[index].~T();
					m_freeList.push_back(index);
				}
			}
		}

		void destroy(HeapAllocator& allocator) override
		{
			allocator.delete_object(this);
		}
	};

	template <t_shared_component T>
	sizet TypedSharedComponentPool<T>::DataHasher::operator()(u32 index) const
	{ return typename T::Hash()(m_pool->m_data[index]); }

	template <t_shared_component T>
	sizet TypedSharedComponentPool<T>::DataHasher::operator()(const T& item) const
	{ return typename T::Hash()(item); }

	template <t_shared_component T>
	bool TypedSharedComponentPool<T>::DataEqual::operator()(u32 indexA, u32 indexB) const
	{
		return typename T::Equals()(m_pool->m_data[indexA], m_pool->m_data[indexB]);
	}

	template <t_shared_component T>
	bool TypedSharedComponentPool<T>::DataEqual::operator()(u32 indexA, const T& itemB) const
	{
		return typename T::Equals()(m_pool->m_data[indexA], itemB);
	}

	template <t_shared_component T>
	bool TypedSharedComponentPool<T>::DataEqual::operator()(const T& itemA, u32 indexB) const
	{
		return typename T::Equals()(itemA, m_pool->m_data[indexB]);
	}

	template <t_shared_component T>
	u32 TypedSharedComponentPool<T>::createUniqueInstance(const T& item)
	{
		u32 newIndex;
		if (!m_freeList.empty())
		{
			newIndex = m_freeList.back();
			m_freeList.pop_back();
			new(&m_data[newIndex]) T(item);
			m_refCounts[newIndex] = 0;
		}
		else
		{
			m_data.push_back(item);
			newIndex = static_cast<u32>(m_data.size() - 1);
			m_refCounts.push_back(0);
		}
		return newIndex;
	}

	class SharedComponentManager
	{
	private:
		HeapAllocator m_allocator;
		heap_unordered_map<ComponentID, ISharedComponentPool*> m_pools;

	public:
		SharedComponentManager(const HeapAllocator& allocator);

		SharedComponentManager(const SharedComponentManager& other) = delete;
		SharedComponentManager(SharedComponentManager&& other) noexcept = delete;
		SharedComponentManager& operator=(const SharedComponentManager& other) = delete;
		SharedComponentManager& operator=(SharedComponentManager&& other) noexcept = delete;

		~SharedComponentManager()
		{
			for (auto& [id, pool] : m_pools)
			{
				pool->destroy(m_allocator);
			}
		}

		template <t_shared_component T>
		SharedComponentHandle getSharedHandle(const T& item = T{})
		{
			TypedSharedComponentPool<T>* pool = getOrCreatePool<T>();
			u32 dataIndex = pool->findOrCreateIndex(item);
			constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<SharedComponent<T>>();
			return {componentId, dataIndex};
		}

		template <t_shared_component T>
		const T& get(SharedComponentHandle handle) const
		{
			SASSERT(handle.componentId == ComponentMetadataRegistry::getComponentId<SharedComponent<T>>())
			const auto* pool = static_cast<const TypedSharedComponentPool<T>*>(m_pools.at(handle.componentId));
			return pool->getData(handle.dataIndex);
		}

		template <t_shared_component T>
		T& getMutable(SharedComponentHandle& handle)
		{
			SASSERT(handle.componentId == ComponentMetadataRegistry::getComponentId<SharedComponent<T>>())
			auto* pool = static_cast<TypedSharedComponentPool<T>*>(m_pools.at(handle.componentId));
			u32 newIndex = pool->getMutableDataIndex(handle.dataIndex);
			handle.dataIndex = newIndex;
			return pool->getDataByIndex(newIndex);
		}

		void incrementRef(SharedComponentHandle handle)
		{
			if (handle.componentId == INVALID_COMPONENT_ID) return;
			m_pools.at(handle.componentId)->incrementRef(handle.dataIndex);
		}

		void decrementRef(SharedComponentHandle handle)
		{
			if (handle.componentId == INVALID_COMPONENT_ID) return;
			m_pools.at(handle.componentId)->decrementRef(handle.dataIndex);
		}
	private:
		template <t_shared_component T>
		TypedSharedComponentPool<T>* getOrCreatePool()
		{
			constexpr ComponentID id = ComponentMetadataRegistry::getComponentId<SharedComponent<T>>();
			auto it = m_pools.find(id);
			if (it != m_pools.end())
			{
				return static_cast<TypedSharedComponentPool<T>*>(it->second);
			}
			auto newPool = m_allocator.new_object<TypedSharedComponentPool<T>>(m_allocator);
			m_pools[id] = newPool;
			return newPool;
		}
	};

}
