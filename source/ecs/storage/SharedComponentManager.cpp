#include "SharedComponentManager.hpp"

#include "ecs/core/ComponentMetadataRegistry.hpp"

namespace spite
{
	SharedComponentManager::SharedComponentManager(
		const HeapAllocator& allocator): m_allocator(allocator),
		                                 m_pools(makeHeapMap<ComponentID, ISharedComponentPool*>(m_allocator))
	{
	}

	SharedComponentHandle::SharedComponentHandle(ComponentID componentId, u32 dataIndex): componentId(componentId),
		dataIndex(dataIndex)
	{
	}
}
