#pragma once
#include "HeapAllocator.hpp"
#include "base/CollectionAliases.hpp"

namespace spite
{
	class AllocatorRegistry
	{
	private:
		glheap_unordered_map<eastl::string, std::unique_ptr<HeapAllocator>> m_allocators;
	public:
		static AllocatorRegistry& instance();

		//creates allocators for main subsystems
		void createSubsystemAllocators();

		// Create a new allocator or return existing one
		HeapAllocator& createAllocator(cstring name, sizet size);

		HeapAllocator& getAllocator(cstring name);

		// Check if an allocator exists
		bool hasAllocator(cstring name) const;

		void shutdownAll();
	};
}
