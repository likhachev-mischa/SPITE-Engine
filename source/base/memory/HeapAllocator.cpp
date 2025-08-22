#include "HeapAllocator.hpp"

#include <cstdlib>
#include <thread>

#include <external/tracy/client/TracyCallstack.hpp>

#include "Base/Assert.hpp"
#include "Base/Logging.hpp"
#include "base/CallstackDebug.hpp"

#include "External/tlsf.h"

namespace spite
{
	constexpr sizet GLOBAL_ALLOCATOR_SIZE = 1024 * MB;

	namespace
	{
		spite::HeapAllocator* s_globalAllocator = nullptr;
	}

	void initGlobalAllocator()
	{
		SASSERTM(!s_globalAllocator, "Trying to initialize the global allocator more than once\n")
		s_globalAllocator = new HeapAllocator("Global Allocator", GLOBAL_ALLOCATOR_SIZE);
	}

	HeapAllocator& getGlobalAllocator()
	{
		SASSERTM(s_globalAllocator, "Global allocator is not initialized\n")
		return *s_globalAllocator;
	}

	void shutdownGlobalAllocator(bool forceDealloc)
	{
		SASSERTM(s_globalAllocator, "Global allocator is not initialized\n")
		s_globalAllocator->shutdown(forceDealloc);
		delete s_globalAllocator;
		s_globalAllocator = nullptr;
	}

	const char* getGlobalAllocatorName()
	{
		SASSERTM(s_globalAllocator, "Global allocator is not initialized\n")
		return s_globalAllocator->get_name();
	}

	struct MemoryStatistics
	{
		sizet allocatedBytes;
		sizet totalBytes;

		u32 allocationCount;

		void add(sizet value);
	};

	void MemoryStatistics::add(sizet value)
	{
		if (value)
		{
			allocatedBytes += value;
			++allocationCount;
		}
	}

	namespace
	{
		void exitWalker(void* ptr, sizet size, int used, void* user)
		{
			auto* stats = static_cast<MemoryStatistics*>(user);
			stats->add(used ? size : 0);

			if (used)
			{
				SDEBUG_LOG("Found active allocation %p, %llu\n", ptr, size)
			}
		}
	}

	HeapAllocator& HeapAllocator::operator=(const HeapAllocator& x)
	{
		return *this;
	}

	HeapAllocator::HeapAllocator(cstring name, sizet size) : m_name(name), m_maxSize(size)
	{
		init(size);
	}

	HeapAllocator::HeapAllocator(const HeapAllocator& x) : m_name(x.m_name),
	                                                       m_tlsfHandle(x.m_tlsfHandle),
	                                                       m_memory(x.m_memory),
	                                                       m_maxSize(x.m_maxSize)
	{
	}

	HeapAllocator::HeapAllocator(const HeapAllocator& x, const char* pName) : m_name(pName),
	                                                                          m_tlsfHandle(x.m_tlsfHandle),
	                                                                          m_memory(x.m_memory),
	                                                                          m_maxSize(x.m_maxSize)

	{
	}

	void* HeapAllocator::allocate(sizet size, int flags) const
	{
		void* mem = tlsf_malloc(m_tlsfHandle, size);
		//SDEBUG_LOG("HeapAllocator %s memory allocation: %p size %llu \n", m_name, mem, size)
		//logCallstack(15, "HeapAlloc stack\n");
		return mem;
	}

	void* HeapAllocator::allocate(sizet size, sizet alignment, sizet offset, int flags) const
	{
		void* mem = tlsf_memalign(m_tlsfHandle, alignment, size);
		//SDEBUG_LOG("HeapAllocator %s memory allocation: %p size %llu \n", m_name, mem, size)
		//logCallstack(15, "HeapAlloc stack\n");
		return mem;
	}

	void* HeapAllocator::reallocate(void* original, sizet size) const
	{
		return tlsf_realloc(m_tlsfHandle, original, size);
	}

	void HeapAllocator::deallocate(void* p, sizet n) const
	{
		tlsf_free(m_tlsfHandle, p);
	}

	const char* HeapAllocator::get_name() const
	{
		return m_name;
	}

	void HeapAllocator::set_name(cstring name)
	{
		m_name = name;
	}

	void HeapAllocator::init(sizet size)
	{
		m_memory = malloc(size);
		m_tlsfHandle = tlsf_create_with_pool(m_memory, size);

		SDEBUG_LOG("HeapAllocator %s of size %llu created\n", m_name, size)
	}

	void HeapAllocator::shutdown(bool forceDealloc) const
	{
		if (forceDealloc)
		{
			tlsf_destroy(m_tlsfHandle);
			free(m_memory);
			return;
		}

		MemoryStatistics stats{.allocatedBytes = 0, .totalBytes = m_maxSize, .allocationCount = 0};
		pool_t pool = tlsf_get_pool(m_tlsfHandle);
		tlsf_walk_pool(pool, exitWalker, &stats);

		if (stats.allocatedBytes > 0)
		{
			SDEBUG_LOG(
				"HeapAllocator %s Shutdown.\n===============\nFAILURE! Allocated memory detected. allocated %llu, total %llu\n===============\n\n",
				m_name,
				stats.allocatedBytes,
				stats.totalBytes)
		}
		else
		{
			SDEBUG_LOG("HeapAllocator %s Shutdown - all memory free!\n", m_name)
		}

		if (stats.allocatedBytes != 0)
		{
			SDEBUG_LOG("Allocations still present\n")
		}
		SASSERTM(stats.allocatedBytes == 0, "Allocations still present\n")

		tlsf_destroy(m_tlsfHandle);
		free(m_memory);
	}

	bool HeapAllocator::operator==(const HeapAllocator& b) const
	{
		return (m_memory == b.m_memory);
	}

	bool HeapAllocator::operator!=(const HeapAllocator& b) const
	{
		return (m_memory != b.m_memory);
	}
}
