#pragma once
#include <cstddef>

#include <EASTL/deque.h>

#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "EASTL/unordered_map.h"

#include "Base/Assert.hpp"
#include "base/memory/Memory.hpp"

namespace spite
{
	// SCRATCH ALLOCATOR - Linear allocator for temporary allocations
	// Perfect for frame-based allocations, string building, temporary containers
	// Very fast allocation (just pointer bump), bulk deallocation (reset)
	class ScratchAllocator
	{
	private:
		char* m_buffer;
		char* m_current;
		char* m_end;
		sizet m_size;
		cstring m_name;

		// Track high water mark for debugging
		mutable sizet m_highWaterMark = 0;

	public:
		explicit ScratchAllocator(size_t buffer_size, const char* name = "ScratchAllocator");

		~ScratchAllocator();

		// Non-copyable, but movable
		ScratchAllocator(const ScratchAllocator&) = delete;
		ScratchAllocator& operator=(const ScratchAllocator&) = delete;

		ScratchAllocator(ScratchAllocator&& other) noexcept;

		ScratchAllocator& operator=(ScratchAllocator&& other) noexcept;

		// Fast pointer-bump allocation
		void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));

		// Individual deallocation is a no-op (linear allocator characteristic)
		void deallocate(void* /*ptr*/, size_t /*size*/) noexcept;

		// Reset to beginning - very fast bulk deallocation
		void reset() noexcept;

		// Get current usage statistics
		sizet bytes_used() const noexcept;
		sizet bytes_remaining() const noexcept;
		sizet total_size() const noexcept;
		sizet high_water_mark() const noexcept;

		const char* get_name() const noexcept;

		// Check if pointer was allocated from this scratch buffer
		bool owns(void* ptr) const noexcept;
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
		using size_type = size_t;
		using difference_type = ptrdiff_t;

		explicit ScratchAllocatorAdapter(ScratchAllocator& allocator) noexcept;

		template <typename U>
		ScratchAllocatorAdapter(const ScratchAllocatorAdapter<U>& other) noexcept;

		ScratchAllocatorAdapter(const ScratchAllocatorAdapter&);
		ScratchAllocatorAdapter& operator=(const ScratchAllocatorAdapter&);

		// Allocator interface
		[[nodiscard]] T* allocate(size_t n);

		void deallocate(T* p, size_t n) noexcept;

		constexpr size_t max_size() const noexcept;

		// EASTL specific requirements
		template <typename U>
		struct rebind
		{
			using type = ScratchAllocatorAdapter<U>;
		};

		// For EASTL compatibility
		void* allocate(size_t n, int /*flags*/  = 0);

		void* allocate(size_t n, size_t alignment, size_t /*offset*/  = 0, int /*flags*/  = 0);

		void deallocate(void* p, size_t n);

		const char* get_name() const;

		void set_name(const char* /*name*/);

		ScratchAllocator* get_allocator() const noexcept;

		// Comparison operators
		bool operator==(const ScratchAllocatorAdapter& other) const noexcept;

		bool operator!=(const ScratchAllocatorAdapter& other) const noexcept;
	};

	template <typename T>
	ScratchAllocatorAdapter<
		T>::ScratchAllocatorAdapter(ScratchAllocator& allocator) noexcept: m_allocator(&allocator)
	{
	}

	template <typename T>
	template <typename U>
	ScratchAllocatorAdapter<T>::ScratchAllocatorAdapter(
		const ScratchAllocatorAdapter<U>& other) noexcept: m_allocator(other.get_allocator())
	{
	}

	template <typename T>
	ScratchAllocatorAdapter<T>::ScratchAllocatorAdapter(const ScratchAllocatorAdapter&) = default;
	template <typename T>
	ScratchAllocatorAdapter<T>& ScratchAllocatorAdapter<T>::operator=(
		const ScratchAllocatorAdapter&) = default;

	template <typename T>
	T* ScratchAllocatorAdapter<T>::allocate(size_t n)
	{
		SASSERTM(n <= max_size(), "Trying to allocate more mem than ScratchAllocator has\n")
		return static_cast<T*>(m_allocator->allocate(n * sizeof(T), alignof(T)));
	}

	template <typename T>
	void ScratchAllocatorAdapter<T>::deallocate(T* p, size_t n) noexcept
	{
		m_allocator->deallocate(p, n * sizeof(T));
	}

	template <typename T>
	constexpr size_t ScratchAllocatorAdapter<T>::max_size() const noexcept
	{
		return std::numeric_limits<size_t>::max() / sizeof(T);
	}

	template <typename T>
	void* ScratchAllocatorAdapter<T>::allocate(size_t n, int)
	{
		return allocate(n);
	}

	template <typename T>
	void* ScratchAllocatorAdapter<T>::allocate(size_t n, size_t alignment, size_t, int)
	{
		return m_allocator->allocate(n, alignment);
	}

	template <typename T>
	void ScratchAllocatorAdapter<T>::deallocate(void* p, size_t n)
	{
		m_allocator->deallocate(p, n);
	}

	template <typename T>
	const char* ScratchAllocatorAdapter<T>::get_name() const
	{
		return m_allocator->get_name();
	}

	template <typename T>
	void ScratchAllocatorAdapter<T>::set_name(const char*)
	{
	}

	template <typename T>
	ScratchAllocator* ScratchAllocatorAdapter<T>::get_allocator() const noexcept
	{
		return m_allocator;
	}

	template <typename T>
	bool ScratchAllocatorAdapter<T>::operator==(const ScratchAllocatorAdapter& other) const noexcept
	{
		return m_allocator == other.m_allocator;
	}

	template <typename T>
	bool ScratchAllocatorAdapter<T>::operator!=(const ScratchAllocatorAdapter& other) const noexcept
	{
		return m_allocator != other.m_allocator;
	}

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

	// Thread-local scratch allocator for per-frame allocations
	class FrameScratchAllocator
	{
	private:
		static constexpr size_t DEFAULT_FRAME_SIZE = MB; // 1MB per frame
		static thread_local ScratchAllocator m_frameAllocator;

	public:
		static ScratchAllocator& get();

		static void resetFrame();

		// Helper to create scratch containers for current frame
		template <typename T>
		static scratch_vector<T> makeVector()
		{
			return scratch_vector<T>(ScratchAllocatorAdapter<T>(get()));
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
