#pragma once
#include "Base/Platform.hpp"
#include <limits>

namespace spite
{
	struct Entity
	{
	private:
		u64 m_id;

	public:
		explicit Entity(const u64 id = 0): m_id(id)
		{
		}

		Entity(u32 index, u32 generation) : m_id(static_cast<u64>(generation) << 32 | index) {}

		[[nodiscard]] u32 index() const { return m_id & 0xFFFFFFFF; }
		[[nodiscard]] u32 generation() const { return m_id >> 32; }
		[[nodiscard]] u64 id() const { return m_id; }

		friend bool operator==(const Entity& lhs, const Entity& rhs) { return lhs.m_id == rhs.m_id; }
		friend bool operator!=(const Entity& lhs, const Entity& rhs) { return lhs.m_id != rhs.m_id; }

		struct hash
		{
			sizet operator()(const spite::Entity& entity) const
			{
				return std::hash<u64>{}(entity.m_id);
			}
		};

		static Entity undefined()
		{
			return Entity(0);
		}

		static constexpr u32 PROXY_GENERATION = std::numeric_limits<u32>::max();
	};
}
