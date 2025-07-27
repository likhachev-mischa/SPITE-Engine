#pragma once
#include "base/DynamicBitset.hpp"
#include "base/CollectionAliases.hpp"
#include "ecs/query/QueryRegistry.hpp"

namespace spite
{
	struct SystemDependencies
	{
		DynamicBitset read;
		DynamicBitset write;
		heap_vector<QueryDescriptor> queries;

		SystemDependencies(const HeapAllocator& allocator)
			: read(allocator),
			  write(allocator),
			  queries(makeHeapVector<QueryDescriptor>(allocator))
		{
		}
	};
}
