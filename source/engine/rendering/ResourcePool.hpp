#pragma once
#include "base/CollectionUtilities.hpp"

namespace spite
{
    // Defines a pool for a specific type of GPU resource.
    template<typename T_Resource>
    struct ResourcePool
    {
        heap_vector<T_Resource> resources;
        heap_vector<u32> generations;
        heap_vector<u32> freeIndices;

        ResourcePool(const HeapAllocator& allocator) 
            : resources(makeHeapVector<T_Resource>(allocator)),
              generations(makeHeapVector<u32>(allocator)),
              freeIndices(makeHeapVector<u32>(allocator)) {}
    };
}
