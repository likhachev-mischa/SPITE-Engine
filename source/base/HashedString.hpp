#pragma once
#include "base/Platform.hpp"
#include <functional>

namespace spite
{
	class StringInterner;

	// A type-safe, lightweight handle for an interned string.
	// It can be passed by value and used as a key in hash maps.
	struct HashedString
	{
	private:
		friend class StringInterner;
		friend struct hash;

		u64 m_id = 0;

		explicit HashedString(u64 id) : m_id(id)
		{
		}

	public:
		HashedString() = default;

		// Resolves the ID back to the original string for debugging or display.
		// Note: This involves a lookup and should not be used in performance-critical code.
		[[nodiscard]] cstring c_str() const;

		u64 id() const { return m_id; }

		bool isValid() const { return m_id != 0; }

		static HashedString undefined() { return HashedString(0); }

		bool operator==(const HashedString& other) const = default;
		bool operator!=(const HashedString& other) const = default;

		struct hash
		{
			sizet operator()(const HashedString& hs) const
			{
				return std::hash<u64>{}(hs.m_id);
			}
		};
	};
}

template <>
struct eastl::hash<spite::HashedString>
{
	sizet operator()(const spite::HashedString& s) const
	{
		return spite::HashedString::hash{}(s);
	}
};
