#pragma once
#include <cstddef>
#include <mutex>

#include <EASTL/deque.h>

#include "Base/Assert.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "base/memory/Memory.hpp"

namespace spite
{
	// SCRATCH ALLOCATOR - Linear allocator for temporary allocations
	// Perfect for frame-based allocations, string building, temporary containers
	// Very fast allocation (just pointer bump), bulk deallocation (reset)
	// Has a backing allocator type : Global or Frame (used for allocating internal buffer)
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
		explicit ScratchAllocator(sizet bufferSize, const char* name = "ScratchAllocator");

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

		template <typename T, typename... Args>
		T* new_object(Args&&... args);

		template <typename T>
		void delete_object(T* obj);

		// Individual deallocation is a no-op (linear allocator characteristic)
		void deallocate(void* /*ptr*/, size_t /*size*/) noexcept;

		// Reset to beginning - very fast bulk deallocation
		void reset() noexcept;

		// RAII helper for managing temporary allocations.
		// Creates a scope that will automatically reset the allocator
		// to its current position upon destruction.
		class ScopedMarker
		{
		private:
			ScratchAllocator* m_allocator;
			char* m_position;

		public:
			~ScopedMarker();

			ScopedMarker(const ScopedMarker&) = delete;
			ScopedMarker& operator=(const ScopedMarker&) = delete;

			ScopedMarker(ScopedMarker&& other) noexcept;

			ScopedMarker& operator=(ScopedMarker&& other) noexcept;

		private:
			friend class ScratchAllocator;

			ScopedMarker(ScratchAllocator* allocator, char* position);
		};

		// Returns a ScopedMarker that will reset the allocator to the current
		// position when the marker goes out of scope.
		// Example:
		// {
		//     auto scoped_alloc = scratch_allocator.get_scoped_marker();
		//     // Perform temporary allocations here...
		// } // Allocator is automatically reset to the marker's position here.
		ScopedMarker get_scoped_marker();

		sizet bytes_used() const noexcept;

		sizet bytes_remaining() const noexcept;

		sizet total_size() const noexcept;

		sizet high_water_mark() const noexcept;

		void print_stats() const noexcept;

		const char* get_name() const noexcept;

		// Check if pointer was allocated from this scratch buffer
		bool owns(const void* ptr) const noexcept;

	private:
		void reset_to(char* position) ;
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

		explicit ScratchAllocatorAdapter(ScratchAllocator& allocator) noexcept;

		template <typename U>
		ScratchAllocatorAdapter(const ScratchAllocatorAdapter<U>& other) noexcept;

		[[nodiscard]] T* allocate(sizet n);

		void deallocate(T* p, sizet n) noexcept;

		constexpr sizet max_size() const noexcept;

		// EASTL specific requirements
		template <typename U>
		struct rebind
		{
			using type = ScratchAllocatorAdapter<U>;
		};

		void* allocate(sizet n, sizet alignment, sizet /*offset*/  = 0, int /*flags*/  = 0);

		void deallocate(void* p, sizet n);

		const char* get_name() const;

		void set_name(const char* /*name*/);

		ScratchAllocator* get_allocator() const noexcept;

		bool operator==(const ScratchAllocatorAdapter& other) const noexcept;

		bool operator!=(const ScratchAllocatorAdapter& other) const noexcept;
	};

	// Thread-local scratch allocator for per-frame allocations
	class FrameScratchAllocator
	{
	private:
		static constexpr sizet DEFAULT_FRAME_SIZE = 32 * MB; // 32MB per frame
		static thread_local ScratchAllocator* m_frameAllocator;

		// Central registry for all created thread-local allocators
		static void* m_allAllocators;
		static std::mutex m_registryMutex;

	public:
		static void init();
		static void shutdown();

		static ScratchAllocator& get();

		static void resetFrame();
	};

	template <typename T, typename ... Args>
	T* ScratchAllocator::new_object(Args&&... args)
	{
		void* ptr = allocate(sizeof(T), alignof(T));
		return new(ptr) T(std::forward<Args>(args)...);
	}

	template <typename T>
	void ScratchAllocator::delete_object(T* obj)
	{
		if (obj)
		{
			obj->~T();
		}
	}

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
	T* ScratchAllocatorAdapter<T>::allocate(sizet n)
	{
		SASSERTM(n <= max_size(), "Trying to allocate more mem than ScratchAllocator has\n")
		return static_cast<T*>(m_allocator->allocate(n * sizeof(T), alignof(T)));
	}

	template <typename T>
	void ScratchAllocatorAdapter<T>::deallocate(T* p, sizet n) noexcept
	{
		//m_allocator->deallocate(p, n * sizeof(T));
	}

	template <typename T>
	constexpr sizet ScratchAllocatorAdapter<T>::max_size() const noexcept
	{
		return std::numeric_limits<sizet>::max() / sizeof(T);
	}

	template <typename T>
	void* ScratchAllocatorAdapter<T>::allocate(sizet n, sizet alignment, sizet, int)
	{
		return m_allocator->allocate(n, alignment);
	}

	template <typename T>
	void ScratchAllocatorAdapter<T>::deallocate(void* p, sizet n)
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
}
