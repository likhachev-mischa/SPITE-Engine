#include "StringInterner.hpp"
#include "base/Assert.hpp"
#include "base/CollectionUtilities.hpp"

namespace spite
{
	StringInterner* StringInterner::m_instance = nullptr;

	void StringInterner::init(HeapAllocator& allocator)
	{
		SASSERTM(!m_instance, "StringInterner is already initialized")
		m_instance = allocator.new_object<StringInterner>(allocator);
		// Pre-register an empty string for the undefined HashedString (ID 0)
		m_instance->m_idToStrings.emplace_back("");
	}

	void StringInterner::destroy()
	{
		SASSERTM(m_instance, "StringInterner is not initialized")
		m_instance->m_allocator.delete_object(m_instance);
		m_instance = nullptr;
	}

	StringInterner* StringInterner::get()
	{
		SASSERTM(m_instance, "StringInterner is not initialized")
		return m_instance;
	}

	sizet StringInterner::StringHasher::operator()(cstring str) const
	{
		// 64-bit FNV-1a constants
		constexpr u64 basis = 14695981039346656037ULL;
		constexpr u64 prime = 1099511628211ULL;

		u64 hash = basis;
		while (*str)
		{
			hash ^= static_cast<u64>(*str++);
			hash *= prime;
		}
		return static_cast<sizet>(hash);
	}

	sizet StringInterner::StringHasher::operator()(const heap_string& txt) const
	{
		return (*this)(txt.c_str());
	}

	bool StringInterner::StringEqual::operator()(const heap_string& a, const heap_string& b) const
	{
		return strcmp(a.c_str(), b.c_str()) == 0;
	}

	bool StringInterner::StringEqual::operator()(const heap_string& a, const char* b) const
	{
		return strcmp(a.c_str(), b) == 0;
	}

	bool StringInterner::StringEqual::operator()(const char* a, const heap_string& b) const
	{
		return strcmp(a, b.c_str()) == 0;
	}

	StringInterner::StringInterner(const HeapAllocator& allocator)
		: m_allocator(allocator),
		  m_idToStrings(makeHeapVector<heap_string>(allocator)),
		  m_stringToId(10, StringHasher{}, StringEqual{},
		               HeapAllocatorAdapter<eastl::pair<const heap_string, u64>>(allocator))
	{
	}

	HashedString StringInterner::getOrCreate(cstring str)
	{
		if (!str || str[0] == '\0')
		{
			return HashedString::undefined();
		}

		std::lock_guard<std::mutex> lock(m_mutex);

		// Heterogeneous lookup using find_as with a const char*
		auto it = m_stringToId.find_as(str,
		                               m_stringToId.hash_function(),
		                               m_stringToId.key_eq());
		if (it != m_stringToId.end())
		{
			return HashedString(it->second);
		}

		// String not found, so now we must create a heap_string to store it.
		heap_string newStr(str);
		u64 newId = m_nextId++;
		m_idToStrings.push_back(newStr);
		m_stringToId[m_idToStrings.back()] = newId; // Use the string in the vector as the key

		//SDEBUG_LOG("CURRENT STRINGS IN INTERNER\n")
		//for (auto& storedStr : m_idToStrings)
		//{
		//	SDEBUG_LOG("%s\t", storedStr.c_str())
		//}
		//SDEBUG_LOG("\n---------\n")

		return HashedString(newId);
	}

	HashedString StringInterner::getOrCreate(const heap_string& str)
	{
		if (str.empty())
		{
			return HashedString::undefined();
		}

		std::lock_guard<std::mutex> lock(m_mutex);

		auto it = m_stringToId.find(str);
		if (it != m_stringToId.end())
		{
			return HashedString(it->second);
		}

		u64 newId = m_nextId++;
		m_idToStrings.push_back(str);
		m_stringToId[m_idToStrings.back()] = newId; // Use the string in the vector as the key
		return HashedString(newId);
	}

	cstring StringInterner::resolve(HashedString id) const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		u64 id_val = id.id();
		if (id_val < m_idToStrings.size())
		{
			return m_idToStrings[id_val].c_str();
		}
		return nullptr;
	}

	// Implementation for HashedString::c_str()
	cstring HashedString::c_str() const
	{
		return StringInterner::get()->resolve(*this);
	}
}
