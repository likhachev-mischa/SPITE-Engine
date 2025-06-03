#include "Memory.hpp"

void* operator new [](sizet size,
                      const char* pName,
                      int flags,
                      unsigned debugFlags,
                      const char* file,
                      int line)
{
#if defined (GLOBAL_ALLOCATOR)
	return spite::getGlobalAllocator().allocate(size);
#else
	return operator new[](size);
#endif
}

void* operator new [](sizet size,
                      sizet alignment,
                      sizet alignmentOffset,
                      const char* pName,
                      int flags,
                      unsigned debugFlags,
                      const char* file,
                      int line)
{
#if defined (GLOBAL_ALLOCATOR)
	return spite::getGlobalAllocator().allocate(size, alignment, alignmentOffset);
#else
	return operator new[](size, static_cast<std::align_val_t>(alignment));
#endif
}

#if defined(SPITE_MEMORY_STACK)
#include"StackWalker.h"
#endif


#if defined (GLOBAL_ALLOCATOR)

#include "HeapAllocator.hpp"

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

	//	BlockAllocator::BlockAllocator(const char* pName) : m_allocator(pName)
	//	{
	//	}
	//
	//	BlockAllocator::BlockAllocator(const BlockAllocator& other): m_allocator(other.m_allocator)
	//	{
	//	}
	//
	//	BlockAllocator& BlockAllocator::operator=(const BlockAllocator& other)
	//	{
	//		shutdown();
	//		m_allocator = other.m_allocator;
	//		return *this;
	//	}
	//
	//	void* BlockAllocator::allocate(sizet size, int flags)
	//	{
	//		void* mem = m_allocator.allocate(size, flags);
	//		//	SDEBUG_LOG("BlockAllocator %s memory allocation: %p size %llu \n", m_allocator.get_name(), mem, size)
	//		return mem;
	//	}
	//
	//	void* BlockAllocator::allocate(sizet size, sizet alignment, sizet offset, int flags)
	//	{
	//		void* mem = m_allocator.allocate(size, alignment, offset, flags);
	//		//	SDEBUG_LOG("BlockAllocator %s memory allocation: %p size %llu \n", m_allocator.get_name(), mem, size)
	//		return mem;
	//	}
	//
	//	void BlockAllocator::deallocate(void* p, sizet n)
	//	{
	//		m_allocator.deallocate(p, n);
	//	}
	//
	//	void BlockAllocator::init(void* pMemory, sizet memorySize, sizet nodeSize, sizet alignment,
	//	                          sizet alignmentOffset)
	//	{
	//		m_totalBytes = memorySize;
	//		m_allocator.init(pMemory, memorySize, nodeSize, alignment, alignmentOffset);
	//		SDEBUG_LOG("BlockAllocator %s created of size %llu, nodeSize %llu created\n", m_allocator.get_name(),
	//		           m_totalBytes, m_allocator.mnNodeSize)
	//	}
	//
	//	void BlockAllocator::shutdown()
	//	{
	//#if defined(DEBUG)
	//		if (m_allocator.mnCurrentSize != 0)
	//		{
	//			SDEBUG_LOG(
	//				"BlockAllocator %s Shutdown.\n===============\nFAILURE! Allocated memory detected. allocated %llu, total %llu\n===============\n\n",
	//				m_allocator.get_name(), m_allocator.mnCurrentSize * m_allocator.mnNodeSize, m_totalBytes)
	//		}
	//		else
	//		{
	//			SDEBUG_LOG("HeapAllocator %s Shutdown - all memory free!\n", m_allocator.get_name())
	//		}
	//		SASSERTM(m_allocator.mnCurrentSize == 0, "Allocations still present. Check your code!\n")
	//#endif
	//	}
}

