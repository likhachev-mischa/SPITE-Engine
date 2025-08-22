#include "SystemDependencyStorage.hpp"

#include "base/CollectionUtilities.hpp"

namespace spite
{
	SystemDependencyStorage::SystemDependencyStorage(const HeapAllocator& allocator): m_systemDependencies(
			makeHeapMap<SystemBase*, SystemDependencies>(allocator)), m_allocator(allocator)
	{
	}

	void SystemDependencyStorage::registerDependencies(SystemBase* system, eastl::span<const ComponentID> reads,
	                                                   eastl::span<const ComponentID> writes)
	{
		auto it = m_systemDependencies.find(system);
		if (it == m_systemDependencies.end())
		{
			it = m_systemDependencies.emplace(system, SystemDependencies(m_allocator)).first;
		}

		auto& deps = it->second;

		for (const auto& componentId : reads)
		{
			SASSERT(componentId != INVALID_COMPONENT_ID)
			deps.read.set(componentId);
		}

		for (const auto& componentId : writes)
		{
			SASSERT(componentId != INVALID_COMPONENT_ID)
			deps.write.set(componentId);
		}
	}

	void SystemDependencyStorage::registerQuery(SystemBase* system, const QueryDescriptor& queryDescriptor)
	{
		auto it = m_systemDependencies.find(system);
		if (it == m_systemDependencies.end())
		{
			it = m_systemDependencies.emplace(system, SystemDependencies(m_allocator)).first;
		}

		auto& deps = it->second;
		deps.queries.push_back(queryDescriptor);

		for (const auto& componentId : queryDescriptor.readAspect->getComponentIds())
		{
			deps.read.set(componentId);
		}

		for (const auto& componentId : queryDescriptor.writeAspect->getComponentIds())
		{
			deps.write.set(componentId);
		}
	}

	const SystemDependencies& SystemDependencyStorage::getDependencies(SystemBase* system)
	{
		auto it = m_systemDependencies.find(system);
		if (it == m_systemDependencies.end())
		{
			it = m_systemDependencies.emplace(system, SystemDependencies(m_allocator)).first;
		}
		return it->second;
	}
}
