#include "AllocatorRegistry.hpp"
#include "base/Logging.hpp"

namespace spite
{
	AllocatorRegistry& AllocatorRegistry::instance()
	{
		static AllocatorRegistry registry;
		return registry;
	}

	void AllocatorRegistry::createSubsystemAllocators()
	{
		createAllocator("MainAllocator",32*MB);
		createAllocator("GpuAllocator", 128 * MB);
	}

	HeapAllocator& AllocatorRegistry::createAllocator(cstring name, sizet size)
	{
		auto it = m_allocators.find(name);
		if (it != m_allocators.end()) {
			SDEBUG_LOG("AllocatorRegistry: Returning existing allocator '%s'\n", name)
			return *it->second;
		}
        
		auto allocator = std::make_unique<HeapAllocator>(name, size);
		HeapAllocator& ref = *allocator;
		m_allocators[name] = std::move(allocator);
        
		SDEBUG_LOG("AllocatorRegistry: Created allocator '%s' with %zu MB\n", 
		           name, size / (MB))
		return ref;
	}

	HeapAllocator& AllocatorRegistry::getAllocator(cstring name)
	{
		auto it = m_allocators.find(name);
		if (it != m_allocators.end()) {
			return *it->second;
		}
		// Fallback to global allocator
		SDEBUG_LOG("AllocatorRegistry: Allocator '%s' not found, using global\n", name)
		return getGlobalAllocator();
	}

	bool AllocatorRegistry::hasAllocator(cstring name) const
	{
		return m_allocators.find(name) != m_allocators.end();
	}

	void AllocatorRegistry::shutdownAll()
	{
		SDEBUG_LOG("AllocatorRegistry: Shutting down %zu subsystem allocators\n", m_allocators.size())
        
		for (auto& [name, allocator] : m_allocators) {
			SDEBUG_LOG("Shutting down allocator: %s\n", name.c_str())
			allocator->shutdown(false);
		}
		m_allocators.clear(true);
	}
}
