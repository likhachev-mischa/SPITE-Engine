#include "Memory.hpp"
#include "Assert.hpp"
#include "Logging.hpp"
#include "External/tlsf.h"

#if defined(SPITE_MEMORY_STACK)
#include"StackWalker.h"
#endif


#if defined (GLOBAL_ALLOCATOR)

void* operator new(sizet size)
{
	return spite::getGlobalAllocator().allocate(size);
}

void operator delete(void* p) noexcept
{
	spite::getGlobalAllocator().deallocate(p);
}

void operator delete(void* p, sizet size) noexcept
{
	spite::getGlobalAllocator().deallocate(p, size);
}

void* operator new[](sizet size)
{
	return spite::getGlobalAllocator().allocate(size);
}

void operator delete[](void* p) noexcept
{
	spite::getGlobalAllocator().deallocate(p);
}

void operator delete[](void* p, sizet size) noexcept
{
	spite::getGlobalAllocator().deallocate(p, size);
}

void* operator new(sizet size, std::align_val_t alignment)
{
	return spite::getGlobalAllocator().allocate(size, static_cast<sizet>(alignment));
}

void* operator new[](sizet size, std::align_val_t alignment)
{
	return spite::getGlobalAllocator().allocate(size, static_cast<sizet>(alignment));
}

void operator delete(void* p, std::align_val_t alignment) noexcept
{
	spite::getGlobalAllocator().deallocate(p);
}

void operator delete[](void* p, std::align_val_t alignment) noexcept
{
	spite::getGlobalAllocator().deallocate(p);
}
#endif
namespace spite
{
	void exitWalker(void* ptr, sizet size, int used, void* user)
	{
		MemoryStatistics* stats = static_cast<MemoryStatistics*>(user);
		stats->add(used ? size : 0);

		if (used)
		{
			SDEBUG_LOG("Found active allocation %p, %llu\n", ptr, size)
		}
	}

	void MemoryStatistics::add(sizet value)
	{
		if (value)
		{
			allocatedBytes += value;
			++allocationCount;
		}
	}

	HeapAllocator& HeapAllocator::operator=(const HeapAllocator& x)
	{
		return *this;
	}

	HeapAllocator::HeapAllocator(const char* pName, sizet size) : m_name(pName), m_maxSize(size)
	{
		init(size);
	}

	HeapAllocator::HeapAllocator(const HeapAllocator& x) : m_name(x.m_name), m_tlsfHandle(x.m_tlsfHandle),
	                                                       m_memory(x.m_memory), m_maxSize(x.m_maxSize)
	{
	}

	HeapAllocator::HeapAllocator(const HeapAllocator& x, const char* pName) : m_name(pName),
	                                                                          m_tlsfHandle(x.m_tlsfHandle),
	                                                                          m_memory(x.m_memory),
	                                                                          m_maxSize(x.m_maxSize)

	{
	}

	//TODO: consider stack walker
#if defined(SPITE_MEMORY_STACK)
	class SpiteStackWalker : public StackWalker
	{
	public:
		SpiteStackWalker() : StackWalker()
		{
		}

	protected:
		void OnOutput(LPCSTR szText) override
		{
			SDEBUG_LOG("\nStack: \n%s\n", szText)
			StackWalker::OnOutput(szText);
		}
	};
#endif

	void* HeapAllocator::allocate(sizet size, int flags)
	{
		void* mem = tlsf_malloc(m_tlsfHandle, size);
		//	SDEBUG_LOG("HeapAllocator %s memory allocation: %p size %llu \n", m_name, mem, size)
		return mem;
	}

	void* HeapAllocator::allocate(sizet size, sizet alignment, sizet offset, int flags)
	{
		void* mem = tlsf_memalign(m_tlsfHandle, alignment, size);
		//	SDEBUG_LOG("HeapAllocator %s memory allocation: %p size %llu \n", m_name, mem, size)
		return mem;
	}

	void* HeapAllocator::reallocate(void* original, sizet size)
	{
		return tlsf_realloc(m_tlsfHandle, original, size);
	}

	void HeapAllocator::deallocate(void* p, sizet n)
	{
		tlsf_free(m_tlsfHandle, p);
	}

	const char* HeapAllocator::get_name() const
	{
		return m_name;
	}

	void HeapAllocator::set_name(const char* pName)
	{
		m_name = pName;
	}

	void HeapAllocator::init(sizet size)
	{
		m_memory = malloc(size);
		m_tlsfHandle = tlsf_create_with_pool(m_memory, size);

		SDEBUG_LOG("HeapAllocator %s of size %llu created\n", m_name, size)
	}

	void HeapAllocator::shutdown(bool forceDealloc)
	{
		if (forceDealloc)
		{
			tlsf_destroy(m_tlsfHandle);
			free(m_memory);
			return;
		}

		MemoryStatistics stats{0, m_maxSize, 0};
		pool_t pool = tlsf_get_pool(m_tlsfHandle);
		tlsf_walk_pool(pool, exitWalker, &stats);

		if (stats.allocatedBytes > 0)
		{
			SDEBUG_LOG(
				"HeapAllocator %s Shutdown.\n===============\nFAILURE! Allocated memory detected. allocated %llu, total %llu\n===============\n\n",
				m_name, stats.allocatedBytes, stats.totalBytes)
		}
		else
		{
			SDEBUG_LOG("HeapAllocator %s Shutdown - all memory free!\n", m_name)
		}

		SASSERTM(stats.allocatedBytes == 0, "Allocations still present. Check your code!\n")

		tlsf_destroy(m_tlsfHandle);
		free(m_memory);
	}

	bool HeapAllocator::operator==(const HeapAllocator& b)
	{
		return (m_memory == b.m_memory);
	}

	bool HeapAllocator::operator!=(const HeapAllocator& b)
	{
		return (m_memory != b.m_memory);
	}

	BlockAllocator::BlockAllocator(const char* pName) : m_allocator(pName)
	{
	}

	BlockAllocator::BlockAllocator(const BlockAllocator& other): m_allocator(other.m_allocator)
	{
	}

	BlockAllocator& BlockAllocator::operator=(const BlockAllocator& other)
	{
		shutdown();
		m_allocator = other.m_allocator;
		return *this;
	}

	void* BlockAllocator::allocate(sizet size, int flags)
	{
		void* mem = m_allocator.allocate(size, flags);
		//	SDEBUG_LOG("BlockAllocator %s memory allocation: %p size %llu \n", m_allocator.get_name(), mem, size)
		return mem;
	}

	void* BlockAllocator::allocate(sizet size, sizet alignment, sizet offset, int flags)
	{
		void* mem = m_allocator.allocate(size, alignment, offset, flags);
		//	SDEBUG_LOG("BlockAllocator %s memory allocation: %p size %llu \n", m_allocator.get_name(), mem, size)
		return mem;
	}

	void BlockAllocator::deallocate(void* p, sizet n)
	{
		m_allocator.deallocate(p, n);
	}

	void BlockAllocator::init(void* pMemory, sizet memorySize, sizet nodeSize, sizet alignment,
	                          sizet alignmentOffset)
	{
		m_totalBytes = memorySize;
		m_allocator.init(pMemory, memorySize, nodeSize, alignment, alignmentOffset);
		SDEBUG_LOG("BlockAllocator %s created of size %llu, nodeSize %llu created\n", m_allocator.get_name(),
		           m_totalBytes, m_allocator.mnNodeSize)
	}

	void BlockAllocator::shutdown()
	{
#if defined(DEBUG)
		if (m_allocator.mnCurrentSize != 0)
		{
			SDEBUG_LOG(
				"BlockAllocator %s Shutdown.\n===============\nFAILURE! Allocated memory detected. allocated %llu, total %llu\n===============\n\n",
				m_allocator.get_name(), m_allocator.mnCurrentSize * m_allocator.mnNodeSize, m_totalBytes)
		}
		else
		{
			SDEBUG_LOG("HeapAllocator %s Shutdown - all memory free!\n", m_allocator.get_name())
		}
		SASSERTM(m_allocator.mnCurrentSize == 0, "Allocations still present. Check your code!\n")
#endif
	}
}
