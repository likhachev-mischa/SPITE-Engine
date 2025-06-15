#include "ScratchAllocator.hpp"

#include "base/memory/HeapAllocator.hpp"

namespace spite
{
	void* GlobalScratchBackingAllocator::allocate(sizet size)
	{
		return getGlobalAllocator().allocate(size);
	}

	void GlobalScratchBackingAllocator::deallocate(void* p, sizet n)
	{
		getGlobalAllocator().deallocate(p, n);
	}

	void* FrameScratchBackingAllocator::allocate(sizet size)
	{
		return FrameScratchAllocator::get().allocate(size);
	}

	void FrameScratchBackingAllocator::deallocate(void* p, sizet n)
	{
		//noop
	}

	ScratchAllocator::ScratchAllocator(sizet bufferSize,
	                                   const char* name,
	                                   ScratchAllocatorType type): m_type(type), m_size(bufferSize),
		m_name(name)
	{
		m_buffer = static_cast<char*>(internalAllocate(bufferSize));


		m_current = m_buffer;
		m_end = m_buffer + bufferSize;
	}

	ScratchAllocator::~ScratchAllocator()
	{
		if (m_buffer)
		{
			internalDeallocate(m_buffer, m_size);
		}
	}

	ScratchAllocator::ScratchAllocator(ScratchAllocator&& other) noexcept: m_type(other.m_type),
		m_buffer(other.m_buffer), m_current(other.m_current), m_end(other.m_end),
		m_size(other.m_size), m_name(other.m_name), m_highWaterMark(other.m_highWaterMark)
	{
		other.m_buffer = nullptr;
		other.m_current = nullptr;
		other.m_end = nullptr;
	}

	ScratchAllocator& ScratchAllocator::operator=(ScratchAllocator&& other) noexcept
	{
		if (this != &other)
		{
			if (m_buffer)
			{
				internalDeallocate(m_buffer, m_size);
			}
			m_type = other.m_type;
			m_buffer = other.m_buffer;
			m_current = other.m_current;
			m_end = other.m_end;
			m_size = other.m_size;
			m_name = other.m_name;
			m_highWaterMark = other.m_highWaterMark;

			other.m_buffer = nullptr;
			other.m_current = nullptr;
			other.m_end = nullptr;
		}
		return *this;
	}

	void* ScratchAllocator::allocate(sizet size, int flags)
	{
		return allocate(size, 16, 0, flags);
	}

	void* ScratchAllocator::allocate(sizet size, sizet alignment, sizet offset, int flags)
	{
		// Align current pointer
		char* aligned = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(m_current) + alignment
			- 1) & ~(alignment - 1));

		SASSERTM(aligned + size <= m_end, "Scratch alloc %s out of memory\n", get_name())

		m_current = aligned + size;

		// Update high water mark
		size_t used = m_current - m_buffer;
		if (used > m_highWaterMark)
		{
			m_highWaterMark = used;
		}

		return aligned;
	}

	void ScratchAllocator::deallocate(void*, size_t) noexcept
	{
		// No-op - use reset() for bulk deallocation
	}

	void ScratchAllocator::reset() noexcept
	{
		m_current = m_buffer;
	}

	sizet ScratchAllocator::bytes_used() const noexcept
	{
		return m_current - m_buffer;
	}

	sizet ScratchAllocator::bytes_remaining() const noexcept
	{
		return m_end - m_current;
	}

	sizet ScratchAllocator::total_size() const noexcept
	{
		return m_size;
	}

	sizet ScratchAllocator::high_water_mark() const noexcept
	{
		return m_highWaterMark;
	}

	void ScratchAllocator::print_stats() const noexcept
	{
		SDEBUG_LOG("Stats for ScratchAllocator %s\n", get_name())
		SDEBUG_LOG("MB used %llu \n", bytes_used() / MB)
		SDEBUG_LOG("MB remaining %llu \n", bytes_remaining() / MB)
		SDEBUG_LOG("Total size %llu \n", total_size() / MB)
		SDEBUG_LOG("High water mark %llu\n", high_water_mark() / MB)
	}

	const char* ScratchAllocator::get_name() const noexcept
	{
		return m_name;
	}

	bool ScratchAllocator::owns(const void* ptr) const noexcept
	{
		return ptr >= m_buffer && ptr < m_end;
	}

	void* ScratchAllocator::internalAllocate(sizet size) const
	{
		switch (m_type)
		{
		case ScratchAllocatorType_Global:
			{
				return GlobalScratchBackingAllocator::allocate(size);
			}
		case ScratchAllocatorType_Frame:
			{
				return FrameScratchBackingAllocator::allocate(size);
			}
		}
		SASSERTM(false, "Scratch allocator allocation error\n")
		return nullptr;
	}

	void ScratchAllocator::internalDeallocate(void* buffer, sizet size)
	{
		switch (m_type)
		{
		case ScratchAllocatorType_Global:
			{
				return GlobalScratchBackingAllocator::deallocate(buffer, size);
			}
		case ScratchAllocatorType_Frame:
			{
				return FrameScratchBackingAllocator::deallocate(buffer, size);
			}
		}
	}

	thread_local ScratchAllocator* FrameScratchAllocator::m_frameAllocator = nullptr;

	void FrameScratchAllocator::init()
	{
		SASSERTM(!m_frameAllocator, "Trying to initialize frame scratch allocator more than once\n")
		m_frameAllocator = new ScratchAllocator(DEFAULT_FRAME_SIZE,
		                                        "FrameScratch",
		                                        ScratchAllocatorType_Global);
	}

	void FrameScratchAllocator::shutdown()
	{
		SASSERTM(m_frameAllocator, "Frame scratch allocator is not initialized\n")
		delete m_frameAllocator;
	}
}
