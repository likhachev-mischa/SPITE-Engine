#pragma once
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

#include "Base/Assert.hpp"
#include "Base/Platform.hpp"
#include "base/memory/Memory.hpp"

namespace spite
{
	//tlsf based heap allocator
	//call shutdown to dispose!
	class HeapAllocator
	{
	public:
		HeapAllocator(HeapAllocator&& other) = delete;
		HeapAllocator& operator=(HeapAllocator&& other) = delete;
		HeapAllocator& operator=(const HeapAllocator& x);

		~HeapAllocator() = default;

		HeapAllocator(cstring name = "HeapAllocator", sizet size = 32 * MB);

		//copied allocator manages the same memory pool
		//create new allocator if otherwise desired
		HeapAllocator(const HeapAllocator& x);
		HeapAllocator(const HeapAllocator& x, const char* pName);

		void* allocate(sizet size, int flags = 0) const;
		void* allocate(sizet size, sizet alignment, sizet offset = 0, int flags = 0) const;

		template <typename T, typename... Args>
		T* new_object(Args&&... args);

		void* reallocate(void* original, sizet size) const;

		template <typename T>
		void delete_object(T* obj);

		void deallocate(void* p, sizet n = 0) const;

		const char* get_name() const;
		void set_name(cstring name);

		/**
		 * \brief disposes of allocations
		 * \param forceDealloc if true, does not check if any allocations remain and silently deallocates anything 
		 */
		void shutdown(bool forceDealloc = false) const;

		bool operator==(const HeapAllocator& b) const;
		bool operator!=(const HeapAllocator& b) const;

	private:
		void init(sizet size);
		cstring m_name;

		void* m_tlsfHandle{};
		void* m_memory{};
		sizet m_maxSize;
	};

	void initGlobalAllocator();
	HeapAllocator& getGlobalAllocator();

	// Optional: Global allocator lifecycle management
	void shutdownGlobalAllocator(bool forceDealloc = false);
	const char* getGlobalAllocatorName();

	//lightweight adaper for HeapAllocator that actually should be passed to collections
	template <typename T>
	class HeapAllocatorAdapter
	{
	private:
		const HeapAllocator* m_allocator;

	public:
		using value_type = T;

		template <typename U>
		struct rebind
		{
			using other = HeapAllocatorAdapter<U>;
		};

		explicit HeapAllocatorAdapter(const HeapAllocator& allocator) noexcept;

		template <typename U>
		explicit HeapAllocatorAdapter(const HeapAllocatorAdapter<U>& other) noexcept;

		T* allocate(sizet size, sizet alignment, sizet offset = 0, int flags = 0) const;
		T* allocate(sizet n);

		void deallocate(T* p, sizet n);
		void deallocate(void* p, sizet n);

		template <typename U>
		bool operator==(const HeapAllocatorAdapter<U>& b) const
		{
			return *m_allocator == *b.m_allocator;
		}
		template <typename U>
		bool operator!=(const HeapAllocatorAdapter<U>& b) const
		{
			return *m_allocator != *b.m_allocator;
		}
	};

	// Stateless allocator that always uses the global allocator
	// This eliminates per-container allocator storage overhead
	template <typename T>
	class GlobalHeapAllocator
	{
	public:
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using sizetype = sizet;
		using difference_type = ptrdiff_t;

		// Stateless - no data members
		constexpr GlobalHeapAllocator() noexcept;
		constexpr GlobalHeapAllocator(cstring) noexcept;
		constexpr GlobalHeapAllocator(const GlobalHeapAllocator&) noexcept;
		constexpr GlobalHeapAllocator(GlobalHeapAllocator&&) noexcept;
		template <typename U>
		constexpr GlobalHeapAllocator(const GlobalHeapAllocator<U>&) noexcept;

		~GlobalHeapAllocator();

		GlobalHeapAllocator& operator=(const GlobalHeapAllocator&);
		GlobalHeapAllocator& operator=(GlobalHeapAllocator&&) noexcept;

		// Allocator interface
		[[nodiscard]] T* allocate(sizet n);

		void deallocate(T* p, sizet n) noexcept;

		constexpr sizet max_size() const noexcept;

		// EASTL specific requirements
		template <typename U>
		struct rebind
		{
			using type = GlobalHeapAllocator<U>;
		};

		// For EASTL compatibility
		void* allocate(sizet n, int flags);

		void* allocate(sizet n, sizet alignment, size_t offset = 0, int flags = 0);

		void deallocate(void* p, sizet n);

		const char* get_name() const;

		void set_name(const char* /*name*/);

		// Comparison operators - stateless allocators always compare equal
		bool operator==(const GlobalHeapAllocator&) const noexcept;
		bool operator!=(const GlobalHeapAllocator&) const noexcept;
	};

	template <typename T>
	constexpr GlobalHeapAllocator<T>::GlobalHeapAllocator() noexcept = default;

	template <typename T>
	constexpr GlobalHeapAllocator<T>::GlobalHeapAllocator(cstring name) noexcept
	{
	};
	template <typename T>
	constexpr GlobalHeapAllocator<T>::GlobalHeapAllocator(const GlobalHeapAllocator&) noexcept
	= default;
	template <typename T>
	constexpr GlobalHeapAllocator<T>::GlobalHeapAllocator(GlobalHeapAllocator&&) noexcept = default;

	template <typename T>
	template <typename U>
	constexpr GlobalHeapAllocator<T>::GlobalHeapAllocator(const GlobalHeapAllocator<U>&) noexcept
	{
	}

	template <typename T, typename... Args>
	T* HeapAllocator::new_object(Args&&... args)
	{
		void* ptr = allocate(sizeof(T), alignof(T));
		return new(ptr) T(std::forward<Args>(args)...);
	}

	template <typename T>
	void HeapAllocator::delete_object(T* obj)
	{
		if (obj)
		{
			obj->~T();
			deallocate(obj, sizeof(T));
		}
	}

	template <typename T>
	HeapAllocatorAdapter<T>::HeapAllocatorAdapter(const HeapAllocator& allocator) noexcept: m_allocator(&allocator)
	{
	}

	template <typename T>
	template <typename U>
	HeapAllocatorAdapter<T>::HeapAllocatorAdapter(const HeapAllocatorAdapter<U>& other) noexcept: m_allocator(
		other.m_allocator)
	{
	}

	template <typename T>
	T* HeapAllocatorAdapter<T>::allocate(sizet size, sizet alignment, sizet offset, int flags) const
	{
		return static_cast<T*>(m_allocator->allocate(size, alignment, offset, flags));
	}

	template <typename T>
	T* HeapAllocatorAdapter<T>::allocate(sizet n)
	{
		return static_cast<T*>(m_allocator->allocate(n * sizeof(T), alignof(T)));
	}

	template <typename T>
	void HeapAllocatorAdapter<T>::deallocate(T* p, sizet n)
	{
		m_allocator->deallocate(p, n * sizeof(T));
	}

	template <typename T>
	void HeapAllocatorAdapter<T>::deallocate(void* p, sizet n)
	{
		m_allocator->deallocate(p, n);
	}

	template <typename T>
	GlobalHeapAllocator<T>::~GlobalHeapAllocator() = default;
	template <typename T>
	GlobalHeapAllocator<T>& GlobalHeapAllocator<T>::operator=(const GlobalHeapAllocator&) = default;
	template <typename T>
	GlobalHeapAllocator<T>& GlobalHeapAllocator<T>::operator=(GlobalHeapAllocator&&) noexcept
	= default;

	template <typename T>
	T* GlobalHeapAllocator<T>::allocate(sizet n)
	{
		SASSERTM(n <= max_size(), "Trying to allocate more mem that Global Allocator has\n")
		return static_cast<T*>(getGlobalAllocator().allocate(n * sizeof(T), alignof(T)));
	}

	template <typename T>
	void GlobalHeapAllocator<T>::deallocate(T* p, sizet n) noexcept
	{
		getGlobalAllocator().deallocate(p, n * sizeof(T));
	}

	template <typename T>
	constexpr sizet GlobalHeapAllocator<T>::max_size() const noexcept
	{
		return std::numeric_limits<sizet>::max() / sizeof(T);
	}

	template <typename T>
	void* GlobalHeapAllocator<T>::allocate(sizet n, int flags)
	{
		return allocate(n);
	}

	template <typename T>
	void* GlobalHeapAllocator<T>::allocate(sizet n, sizet alignment, size_t offset, int flags)
	{
		return getGlobalAllocator().allocate(n, alignment, offset, flags);
	}

	template <typename T>
	void GlobalHeapAllocator<T>::deallocate(void* p, sizet n)
	{
		getGlobalAllocator().deallocate(p, n);
	}

	template <typename T>
	const char* GlobalHeapAllocator<T>::get_name() const
	{
		return "GlobalHeapAllocator";
	}

	template <typename T>
	void GlobalHeapAllocator<T>::set_name(const char*)
	{
	}

	template <typename T>
	bool GlobalHeapAllocator<T>::operator==(const GlobalHeapAllocator&) const noexcept
	{
		return true;
	}

	template <typename T>
	bool GlobalHeapAllocator<T>::operator!=(const GlobalHeapAllocator&) const noexcept
	{
		return false;
	}
}
