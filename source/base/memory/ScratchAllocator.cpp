#include "ScratchAllocator.hpp"

#include "base/memory/HeapAllocator.hpp"

namespace spite
{
	ScratchAllocator::ScratchAllocator(size_t buffer_size, const char* name): m_size(buffer_size),
		m_name(name)
	{
		m_buffer = static_cast<char*>(getGlobalAllocator().allocate(buffer_size, 16));
		// 16-byte aligned
		m_current = m_buffer;
		m_end = m_buffer + buffer_size;
	}

	ScratchAllocator::~ScratchAllocator()
	{
		if (m_buffer)
		{
			getGlobalAllocator().deallocate(m_buffer, m_size);
		}
	}

	ScratchAllocator::ScratchAllocator(ScratchAllocator&& other) noexcept: m_buffer(other.m_buffer),
		m_current(other.m_current), m_end(other.m_end), m_size(other.m_size), m_name(other.m_name),
		m_highWaterMark(other.m_highWaterMark)
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
				getGlobalAllocator().deallocate(m_buffer, m_size);
			}
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

	void* ScratchAllocator::allocate(size_t size, size_t alignment)
	{
		// Align current pointer
		char* aligned = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(m_current) + alignment
			- 1) & ~(alignment - 1));

		SASSERTM(aligned + size <= m_end, "Scratch alloc % out of memory\n", get_name())

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

	size_t ScratchAllocator::bytes_used() const noexcept
	{
		return m_current - m_buffer;
	}

	size_t ScratchAllocator::bytes_remaining() const noexcept
	{
		return m_end - m_current;
	}

	size_t ScratchAllocator::total_size() const noexcept
	{
		return m_size;
	}

	size_t ScratchAllocator::high_water_mark() const noexcept
	{
		return m_highWaterMark;
	}

	const char* ScratchAllocator::get_name() const noexcept
	{
		return m_name;
	}

	bool ScratchAllocator::owns(void* ptr) const noexcept
	{
		return ptr >= m_buffer && ptr < m_end;
	}

    // Define the thread_local storage
    thread_local ScratchAllocator FrameScratchAllocator::m_frameAllocator{DEFAULT_FRAME_SIZE, "FrameScratch"};

	ScratchAllocator& FrameScratchAllocator::get()
	{
		return m_frameAllocator;
	}

	void FrameScratchAllocator::resetFrame()
	{
		m_frameAllocator.reset();
	}
}
