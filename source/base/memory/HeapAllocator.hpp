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


		void* allocate(sizet size, int flags = 0);
		void* allocate(sizet size, sizet alignment, sizet offset = 0, int flags = 0);
		void* reallocate(void* original, sizet size);
		void deallocate(void* p, sizet n = 0);

		const char* get_name() const;
		void set_name(cstring name);


		/**
		 * \brief disposes of allocations
		 * \param forceDealloc if true, does not check if any allocations remain and silently deallocates anything 
		 */
		void shutdown(bool forceDealloc = false);

	protected:
		bool operator==(const HeapAllocator& b);
		bool operator!=(const HeapAllocator& b);

	private:
		void init(sizet size);
		cstring m_name;

		void* m_tlsfHandle{};
		void* m_memory{};
		sizet m_maxSize;
	};

	//Type aliases for heap allocated collections
	template <typename T>
	using heap_vector = eastl::vector<T, HeapAllocator>;

	template <typename T>
	using heap_string = eastl::basic_string<T, HeapAllocator>;

	using heap_string8 = heap_string<char>;
	using heap_string16 = heap_string<char16_t>;
	using heap_string32 = heap_string<char32_t>;

	template <typename Key, typename Value>
	using heap_unordered_map = eastl::unordered_map<Key, Value,
	                                                eastl::hash<Key>, eastl::equal_to<Key>,
	                                                HeapAllocator>;

	spite::HeapAllocator& getGlobalAllocator();

	// Optional: Global allocator lifecycle management
	void shutdownGlobalAllocator(bool force_cleanup = false);
	const char* getGlobalAllocatorName();

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
		using size_type = size_t;
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
		[[nodiscard]] T* allocate(size_t n);

		void deallocate(T* p, size_t n) noexcept;

		constexpr size_t max_size() const noexcept;

		// EASTL specific requirements
		template <typename U>
		struct rebind
		{
			using type = GlobalHeapAllocator<U>;
		};

		// For EASTL compatibility
		void* allocate(size_t n,int flags);

		void* allocate(size_t n, size_t alignment, size_t offset = 0, int flags = 0);

		void deallocate(void* p, size_t n);

		const char* get_name() const;

		void set_name(const char* /*name*/);

		// Comparison operators - stateless allocators always compare equal
		bool operator==(const GlobalHeapAllocator&) const noexcept;
		bool operator!=(const GlobalHeapAllocator&) const noexcept;
	};

	//Type aliases for global heap allocated collections
	template <typename T>
	using glheap_vector = eastl::vector<T, GlobalHeapAllocator<T>>;

	template <typename T>
	using glheap_string = eastl::basic_string<T, GlobalHeapAllocator<T>>;

	using glheap_string8 = glheap_string<char>;
	using glheap_string16 = glheap_string<char16_t>;
	using glheap_string32 = glheap_string<char32_t>;

	template <typename Key, typename Value>
	using glheap_unordered_map = eastl::unordered_map<Key, Value,
	                                                  eastl::hash<Key>, eastl::equal_to<Key>,
	                                                  GlobalHeapAllocator<eastl::pair<
		                                                  const Key, Value>>>;

	template <typename T>
	constexpr GlobalHeapAllocator<T>::GlobalHeapAllocator() noexcept = default;
	template <typename T>
	constexpr GlobalHeapAllocator<T>::GlobalHeapAllocator(cstring name) noexcept {};
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

	template <typename T>
	GlobalHeapAllocator<T>::~GlobalHeapAllocator() = default;
	template <typename T>
	GlobalHeapAllocator<T>& GlobalHeapAllocator<T>::operator=(const GlobalHeapAllocator&) = default;
	template <typename T>
	GlobalHeapAllocator<T>& GlobalHeapAllocator<T>::operator=(GlobalHeapAllocator&&) noexcept = default;

	template <typename T>
	T* GlobalHeapAllocator<T>::allocate(size_t n)
	{
		SASSERTM(n <= max_size(), "Trying to allocate more mem that GlobalAllocator has\n")
		return static_cast<T*>(getGlobalAllocator().allocate(n * sizeof(T), alignof(T)));
	}

	template <typename T>
	void GlobalHeapAllocator<T>::deallocate(T* p, size_t n) noexcept
	{
		getGlobalAllocator().deallocate(p, n * sizeof(T));
	}

	template <typename T>
	constexpr size_t GlobalHeapAllocator<T>::max_size() const noexcept
	{
		return std::numeric_limits<size_t>::max() / sizeof(T);
	}

	template <typename T>
	void* GlobalHeapAllocator<T>::allocate(size_t n, int flags)
	{
		return allocate(n);
	}

	template <typename T>
	void* GlobalHeapAllocator<T>::allocate(size_t n, size_t alignment, size_t offset, int flags)
	{
		return getGlobalAllocator().allocate(n, alignment, offset, flags);
	}

	template <typename T>
	void GlobalHeapAllocator<T>::deallocate(void* p, size_t n)
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
