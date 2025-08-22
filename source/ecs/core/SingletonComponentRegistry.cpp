#include "SingletonComponentRegistry.hpp"

#include "base/CollectionUtilities.hpp"

namespace spite
{
	SingletonComponentRegistry::SingletonComponentRegistry(const HeapAllocator& allocator)
		: m_instances(
			  makeHeapMap<std::type_index, std::unique_ptr<ISingletonWrapper>, std::hash<std::type_index>>(allocator)),
		  m_mutexes(makeHeapMap<std::type_index, std::unique_ptr<std::mutex>, std::hash<std::type_index>>(allocator))
	{
	}
}
