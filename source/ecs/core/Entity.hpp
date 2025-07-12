#pragma once
#include "Base/Platform.hpp"

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

		u64 id() const
		{
			return m_id;
		}

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
	};
}
