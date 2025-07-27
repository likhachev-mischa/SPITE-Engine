#include "ScratchAllocator.hpp"

#include <algorithm>
#include <mutex>

#include "base/CollectionAliases.hpp"
#include "base/CollectionUtilities.hpp"
#include "base/memory/HeapAllocator.hpp"

namespace spite
{
	ScratchAllocator::ScratchAllocator(sizet bufferSize, const char* name): m_size(bufferSize),
	                                                                        m_name(name)
	{
		m_buffer = static_cast<char*>(getGlobalAllocator().allocate(bufferSize));


		m_current = m_buffer;
		m_end = m_buffer + bufferSize;
	}

	ScratchAllocator::~ScratchAllocator()
	{
		if (m_buffer)
		{
			getGlobalAllocator().deallocate(m_buffer, m_size);
		}
	}

	ScratchAllocator::ScratchAllocator(ScratchAllocator&& other) noexcept:
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

	void* ScratchAllocator::allocate(sizet size, int flags)
	{
		return allocate(size, 16, 0, flags);
	}

	void* ScratchAllocator::allocate(sizet size, sizet alignment, sizet offset, int flags)
	{
		void* ptr = m_current;
		size_t space = m_end - m_current;

		if (std::align(alignment, size, ptr, space))
		{
			m_current = static_cast<char*>(ptr) + size;

			// Update high water mark
			size_t used = m_current - m_buffer;
			m_highWaterMark = std::max(used, m_highWaterMark);

			return ptr;
		}

		SASSERTM(false, "Scratch alloc %s out of memory\n", get_name())
		return nullptr;
	}

	void ScratchAllocator::deallocate(void*, size_t) noexcept
	{
		// No-op - use reset() for bulk deallocation
	}

	void ScratchAllocator::reset() noexcept
	{
		m_current = m_buffer;
	}

	ScratchAllocator::ScopedMarker::~ScopedMarker()
	{
		if (m_allocator)
		{
			m_allocator->reset_to(m_position);
		}
	}

	ScratchAllocator::ScopedMarker::ScopedMarker(ScopedMarker&& other) noexcept: m_allocator(other.m_allocator),
		m_position(other.m_position)
	{
		other.m_allocator = nullptr; // Prevent double-reset
	}

	ScratchAllocator::ScopedMarker& ScratchAllocator::ScopedMarker::operator=(
		ScopedMarker&& other) noexcept
	{
		if (this != &other)
		{
			if (m_allocator)
			{
				m_allocator->reset_to(m_position);
			}
			m_allocator = other.m_allocator;
			m_position = other.m_position;
			other.m_allocator = nullptr;
		}
		return *this;
	}

	ScratchAllocator::ScopedMarker::ScopedMarker(ScratchAllocator* allocator, char* position): m_allocator(allocator),
		m_position(position)
	{
	}

	ScratchAllocator::ScopedMarker ScratchAllocator::get_scoped_marker()
	{
		return ScopedMarker(this, m_current);
	}

	void ScratchAllocator::reset_to(char* position)
	{
		SASSERTM(position >= m_buffer && position < m_end,
		         "Position is out of bounds of scratch allocator")
		m_current = position;
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

	thread_local ScratchAllocator* FrameScratchAllocator::m_frameAllocator = nullptr;
	void* FrameScratchAllocator::m_allAllocators = nullptr;
	std::mutex FrameScratchAllocator::m_registryMutex;

	void FrameScratchAllocator::init()
	{
		SASSERTM(!m_allAllocators, "FrameScratchAllocators were already initialized\n")
		m_allAllocators = getGlobalAllocator().new_object<heap_vector<ScratchAllocator*>>(
			makeHeapVector<ScratchAllocator*>(getGlobalAllocator()));
	}

	void FrameScratchAllocator::shutdown()
	{
		SASSERTM(m_allAllocators, "FrameScratchAllocators were not initialized\n")
		std::lock_guard<std::mutex> lock(m_registryMutex);
		if (m_allAllocators)
		{
			auto allocators = static_cast<heap_vector<ScratchAllocator*>*>(m_allAllocators);
			for (auto* alloc : *allocators)
			{
				getGlobalAllocator().delete_object(alloc);
			}
			getGlobalAllocator().delete_object(allocators);
			m_allAllocators = nullptr;
		}
		// The thread_local m_frameAllocator pointers will be destroyed automatically.
	}

	ScratchAllocator& FrameScratchAllocator::get()
	{
		if (!m_frameAllocator)
		{
			m_frameAllocator = getGlobalAllocator().new_object<ScratchAllocator>(
				ScratchAllocator(DEFAULT_FRAME_SIZE, "FrameScratchThreadAllocator"));

			// Register the new allocator in the central list for later cleanup
			std::lock_guard<std::mutex> lock(m_registryMutex);
			static_cast<heap_vector<ScratchAllocator*>*>(m_allAllocators)->push_back(m_frameAllocator);
		}
		return *m_frameAllocator;
	}

	void FrameScratchAllocator::resetFrame()
	{
		std::lock_guard<std::mutex> lock(m_registryMutex);
		SASSERTM(m_allAllocators, "FrameScratchAllocators were not initialized\n")
		auto& allocators = *static_cast<heap_vector<ScratchAllocator*>*>(m_allAllocators);
		for (auto allocator : allocators)
		{
			allocator->reset();
		}
	}
}
