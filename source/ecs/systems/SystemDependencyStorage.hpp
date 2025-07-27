#pragma once

#include <EASTL/span.h>

#include "base/CollectionAliases.hpp"

#include "ecs/core/ComponentMetadata.hpp"
#include "ecs/systems/SystemDependencies.hpp"

namespace spite
{
	class SystemBase;

	class SystemDependencyStorage
	{
	private:
		heap_unordered_map<SystemBase*, SystemDependencies> m_systemDependencies;
		HeapAllocator m_allocator;

	public:
		SystemDependencyStorage(const HeapAllocator& allocator);

		void registerDependencies(SystemBase* system, eastl::span<const ComponentID> reads,
		                          eastl::span<const ComponentID> writes);

		void registerQuery(SystemBase* system, const QueryDescriptor& queryDescriptor);

		const SystemDependencies& getDependencies(SystemBase* system);
	};
}
