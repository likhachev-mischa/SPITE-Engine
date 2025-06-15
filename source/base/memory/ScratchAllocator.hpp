#pragma once
#include <cstddef>

#include <EASTL/deque.h>

#include "Base/Assert.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "base/memory/Memory.hpp"

#include "EASTL/string.h"
#include "EASTL/unordered_map.h"
#include "EASTL/vector.h"

namespace spite
{
	// global HeapAllocator interface for ScratchAllocator
	struct GlobalScratchBackingAllocator
	{
		static void* allocate(sizet size);

		static void deallocate(void* p, sizet n);
	};

	// FrameScratchAllocator interface for creating other ScratchAllocators
	struct FrameScratchBackingAllocator
	{
		static void* allocate(sizet size);

		static void deallocate(void* p, sizet n);
	};

	enum ScratchAllocatorType
	{
		ScratchAllocatorType_Global = 0,
		ScratchAllocatorType_Frame = 1
	};

	// SCRATCH ALLOCATOR - Linear allocator for temporary allocations
	// Perfect for frame-based allocations, string building, temporary containers
	// Very fast allocation (just pointer bump), bulk deallocation (reset)
	// Has a backing allocator type : Global or Frame (used for allocating internal buffer)
	class ScratchAllocator
	{
	private:
		//crude runtime polymorphism replacement
		ScratchAllocatorType m_type;
		char* m_buffer;
		char* m_current;
		char* m_end;
		sizet m_size;
		cstring m_name;

		// Track high water mark for debugging
		mutable sizet m_highWaterMark = 0;

	public:
		explicit ScratchAllocator(sizet bufferSize,
		                          const char* name = "ScratchAllocator",
		                          ScratchAllocatorType type = ScratchAllocatorType_Global);

		~ScratchAllocator();

		// Non-copyable, but movable
		ScratchAllocator(const ScratchAllocator&) = delete;
		ScratchAllocator& operator=(const ScratchAllocator&) = delete;

		ScratchAllocator(ScratchAllocator&& other) noexcept;

		ScratchAllocator& operator=(ScratchAllocator&& other) noexcept;

		//alignment is 16 by default
		void* allocate(sizet size, int flags = 0);

		// Fast pointer-bump allocation
		void* allocate(sizet size, sizet alignment, sizet offset = 0, int flags = 0);

		// Individual deallocation is a no-op (linear allocator characteristic)
		void deallocate(void* /*ptr*/, size_t /*size*/) noexcept;

		// Reset to beginning - very fast bulk deallocation
		void reset() noexcept;

		sizet bytes_used() const noexcept;

		sizet bytes_remaining() const noexcept;

		sizet total_size() const noexcept;

		sizet high_water_mark() const noexcept;

		void print_stats() const noexcept;

		const char* get_name() const noexcept;

		// Check if pointer was allocated from this scratch buffer
		bool owns(const void* ptr) const noexcept;

	private:
		void* internalAllocate(sizet size) const;

		void internalDeallocate(void* buffer, sizet size);
	};

	// EASTL-compatible wrapper for ScratchAllocator
	template <typename T>
	class ScratchAllocatorAdapter
	{
	private:
		ScratchAllocator* m_allocator;

	public:
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using difference_type = ptrdiff_t;

		explicit ScratchAllocatorAdapter(ScratchAllocator& allocator) noexcept : m_allocator(
			&allocator)
		{
		}

		template <typename U>
		ScratchAllocatorAdapter(const ScratchAllocatorAdapter<U>& other) noexcept : m_allocator(
			other.get_allocator())
		{
		}

		[[nodiscard]] T* allocate(sizet n)
		{
			SASSERTM(n <= max_size(), "Trying to allocate more mem than ScratchAllocator has\n")
			return static_cast<T*>(m_allocator->allocate(n * sizeof(T), alignof(T)));
		}

		void deallocate(T* p, sizet n) noexcept
		{
			//m_allocator->deallocate(p, n * sizeof(T));
		}

		constexpr sizet max_size() const noexcept
		{
			return std::numeric_limits<sizet>::max() / sizeof(T);
		}

		// EASTL specific requirements
		template <typename U>
		struct rebind
		{
			using type = ScratchAllocatorAdapter<U>;
		};

		void* allocate(sizet n, sizet alignment, sizet /*offset*/  = 0, int /*flags*/  = 0)
		{
			return m_allocator->allocate(n, alignment);
		}

		void deallocate(void* p, sizet n)
		{
			m_allocator->deallocate(p, n);
		}

		const char* get_name() const
		{
			return m_allocator->get_name();
		}

		void set_name(const char* /*name*/)
		{
		}

		ScratchAllocator* get_allocator() const noexcept
		{
			return m_allocator;
		}

		bool operator==(const ScratchAllocatorAdapter& other) const noexcept
		{
			return m_allocator == other.m_allocator;
		}

		bool operator!=(const ScratchAllocatorAdapter& other) const noexcept
		{
			return m_allocator != other.m_allocator;
		}
	};

	// Convenience typedefs for scratch-allocated EASTL containers
	template <typename T>
	using scratch_vector = eastl::vector<T, ScratchAllocatorAdapter<T>>;

	template <typename T>
	using scratch_string = eastl::basic_string<T, ScratchAllocatorAdapter<T>>;

	using scratch_string8 = scratch_string<char>;
	using scratch_string16 = scratch_string<char16_t>;
	using scratch_string32 = scratch_string<char32_t>;

	template <typename Key, typename Value>
	using scratch_unordered_map = eastl::unordered_map<Key, Value,
	                                                   eastl::hash<Key>, eastl::equal_to<Key>,
	                                                   ScratchAllocatorAdapter<eastl::pair<
		                                                   const Key, Value>>>;

	template <typename T>
	inline static scratch_vector<T> makeScratchVector(ScratchAllocator& allocator)
	{
		return scratch_vector<T>(ScratchAllocatorAdapter<T>(allocator));
	}

	// Thread-local scratch allocator for per-frame allocations
	class FrameScratchAllocator
	{
	private:
		static constexpr sizet DEFAULT_FRAME_SIZE = 32 * MB; // 32MB per frame
		static thread_local ScratchAllocator* m_frameAllocator;

	public:
		static void init();
		static void shutdown();

		static ScratchAllocator& get()
		{
			SASSERTM(m_frameAllocator, "Frame scratch allocator is not initialized\n")
			return *m_frameAllocator;
		}

		static void resetFrame()
		{
			SASSERTM(m_frameAllocator, "Frame scratch allocator is not initialized\n")
			m_frameAllocator->reset();
		}

		// Helper to create scratch containers for current frame
		template <typename T>
		static scratch_vector<T> makeVector()
		{
			//return scratch_vector<T>(ScratchAllocatorAdapter<T>(get()));
			return makeScratchVector<T>(get());
		}

		template <typename Key, typename Value>
		static scratch_unordered_map<Key, Value> makeMap()
		{
			using PairType = eastl::pair<const Key, Value>;
			return scratch_unordered_map<Key, Value>(0,
			                                         eastl::hash<Key>(),
			                                         eastl::equal_to<Key>(),
			                                         ScratchAllocatorAdapter<PairType>(get()));
		}

		static scratch_string8 makeString()
		{
			return scratch_string8(ScratchAllocatorAdapter<char>(get()));
		}
	};
}
