#pragma once
#include "HeapAllocator.hpp"

namespace spite
{
	class AllocatorRegistry
	{
	private:
		glheap_unordered_map<eastl::string, std::unique_ptr<HeapAllocator>> m_allocators;
	public:
		static AllocatorRegistry& instance();

		// Create a new allocator or return existing one
		HeapAllocator& createAllocator(cstring name, size_t size);

		HeapAllocator& getAllocator(cstring name);

		// Check if an allocator exists
		bool hasAllocator(cstring name) const;

		// Get statistics for all allocators
		void printStatistics() const;

		void shutdownAll();
	};
}
