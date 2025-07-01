#pragma once
#include "base/CollectionAliases.hpp"

namespace spite
{
	template <typename T>
	heap_vector<T> makeHeapVector(const HeapAllocator& allocator)
	{
		return heap_vector<T>(HeapAllocatorAdapter<T>(allocator));
	}

	template <typename Key, typename Value, typename Hash = eastl::hash<Key>>
	heap_unordered_map<Key,Value, Hash> makeHeapMap(const HeapAllocator& allocator)
	{
		return heap_unordered_map<Key, Value, Hash>(HeapAllocatorAdapter<eastl::pair<const Key, Value>>(allocator));
	}

	template <typename T, sizet C>
	heap_sbo_vector<T, C> makeSboVector(const HeapAllocator& allocator)
	{
		return heap_sbo_vector<T, C>(HeapAllocatorAdapter<T>(allocator));
	}

	template <typename T>
	scratch_vector<T> makeScratchVector(ScratchAllocator& allocator)
	{
		return scratch_vector<T>(ScratchAllocatorAdapter<T>(allocator));
	}

	template <typename Key, typename Value>
	scratch_unordered_map<Key, Value> makeScratchMap(ScratchAllocator& allocator)
	{
		using PairType = eastl::pair<const Key, Value>;
		return scratch_unordered_map<Key, Value>(0,
		                                         eastl::hash<Key>(),
		                                         eastl::equal_to<Key>(),
		                                         ScratchAllocatorAdapter<PairType>(allocator));
	}

	inline scratch_string8 makeScratchString(ScratchAllocator& allocator)
	{
		return scratch_string8(ScratchAllocatorAdapter<char>(allocator));
	}
}
