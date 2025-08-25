#pragma once
#include <EASTL/set.h>
#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

#include "base/Collections.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "base/memory/ScratchAllocator.hpp"

namespace spite
{
	//Type aliases for heap allocated collections
	template <typename T>
	using heap_vector = eastl::vector<T, HeapAllocatorAdapter<T>>;

	template <typename T = char>
	using heap_string_t = eastl::basic_string<T, HeapAllocatorAdapter<T>>;

	using heap_string = heap_string_t<char>;

	using heap_string8 = heap_string_t<char>;
	using heap_string16 = heap_string_t<char16_t>;
	using heap_string32 = heap_string_t<char32_t>;

	template <typename Key, typename Value, typename Hash = eastl::hash<Key>, typename Equal = eastl::equal_to<Key>>
	using heap_unordered_map = eastl::unordered_map<Key, Value,
	                                                Hash, Equal, HeapAllocatorAdapter
	                                                <eastl::pair<const Key, Value>>>;
	template <typename T>
	using heap_set = eastl::vector_set<T, eastl::less<T>, HeapAllocatorAdapter<T>>;

	//Type aliases for global heap allocated collections
	template <typename T>
	using glheap_vector = eastl::vector<T, GlobalHeapAllocator<T>>;

	template <typename T>
	using glheap_string = eastl::basic_string<T, GlobalHeapAllocator<T>>;

	using glheap_string8 = glheap_string<char>;
	using glheap_string16 = glheap_string<char16_t>;
	using glheap_string32 = glheap_string<char32_t>;

	template <typename Key, typename Value, typename Hash = eastl::hash<Key>>
	using glheap_unordered_map = eastl::unordered_map<Key, Value,
	                                                  Hash, eastl::equal_to<Key>,
	                                                  GlobalHeapAllocator<eastl::pair<
		                                                  const Key, Value>>>;

	template <typename T, sizet C>
	using heap_sbo_vector = sbo_vector<T, C, HeapAllocatorAdapter<T>>;

	// Convenience typedefs for scratch-allocated EASTL containers
	template <typename T>
	using scratch_vector = eastl::vector<T, ScratchAllocatorAdapter<T>>;

	template <typename T>
	using scratch_string = eastl::basic_string<T, ScratchAllocatorAdapter<T>>;

	using scratch_string8 = scratch_string<char>;
	using scratch_string16 = scratch_string<char16_t>;
	using scratch_string32 = scratch_string<char32_t>;

	template <typename T>
	using scratch_set = eastl::vector_set<T, eastl::less<T>, ScratchAllocatorAdapter<T>>;

	template <typename Key, typename Value, typename Hash = eastl::hash<Key>>
	using scratch_unordered_map = eastl::unordered_map<Key, Value,
	                                                   Hash, eastl::equal_to<Key>,
	                                                   ScratchAllocatorAdapter<eastl::pair<
		                                                   const Key, Value>>>;
}

namespace eastl
{
	/// Hash specialization for eastl::basic_string with custom allocators.
	/// The hash of a string should only depend on its contents, not its allocator.
	/// This forwards to EASTL's internal string hashing function, which correctly
	/// hashes the character sequence. This makes all of spite::heap_string,
	/// spite::scratch_string, etc. usable as keys in unordered_maps by default.
	template <typename T, typename Allocator>
	struct hash<eastl::basic_string<T, Allocator>>
	{
		sizet operator()(const eastl::basic_string<T, Allocator>& s) const
		{
			return string_hash<eastl::basic_string<T, Allocator>>()(s);
		}
	};
}
