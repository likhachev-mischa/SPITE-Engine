#pragma once
#include "base/CollectionAliases.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "HashedString.hpp"
#include <mutex>


namespace spite
{
	// A thread-safe singleton registry for interning strings.
	// It maps strings to unique HashedString IDs to save memory and improve lookup performance.
	class StringInterner
	{
	private:
		// Transparent hasher for heterogeneous lookups (char* vs heap_string)
		struct StringHasher
		{
			using is_transparent = void;

			// Hasher for the lookup type (const char*)
			// Implements the FNV-1a hash algorithm
			sizet operator()(cstring str) const;

			// Hasher for the actual key type (eastl::string)
			sizet operator()(const heap_string& txt) const;
		};

		// Transparent equality for heterogeneous lookups
		struct StringEqual
		{
			using is_transparent = void;

			// Explicitly use strcmp for all comparisons to guarantee content-based checking.
			bool operator()(const heap_string& a, const heap_string& b) const;

			bool operator()(const heap_string& a, const char* b) const;
			bool operator()(const char* a, const heap_string& b) const;
		};

		static StringInterner* m_instance;
		HeapAllocator m_allocator;

		// Owns the actual string data
		heap_vector<heap_string> m_idToStrings;
		// Maps a string to its unique ID for fast lookups
		heap_unordered_map<heap_string, u64, StringHasher, StringEqual> m_stringToId;

		u64 m_nextId = 1; // Start from 1, 0 is reserved for undefined/empty
		mutable std::mutex m_mutex;

	public:
		StringInterner(const StringInterner&) = delete;
		StringInterner& operator=(const StringInterner&) = delete;
		StringInterner(const HeapAllocator& allocator);

		static void init(HeapAllocator& allocator);
		static void destroy();
		static StringInterner* get();

		// Gets the HashedString for a given C-style string.
		// This version avoids intermediate heap_string allocation for lookups.
		HashedString getOrCreate(cstring str);

		// Gets the HashedString for a given eastl::string.
		// use cstring version whenever possible
		HashedString getOrCreate(const heap_string& str);

		// Resolves a HashedString back to its original C-style string.
		// Returns nullptr if the ID is invalid.
		cstring resolve(HashedString id) const;
	};

	//convenient function
	inline HashedString toHashedString(cstring str)
	{
		return StringInterner::get()->getOrCreate(str);
	}
}
