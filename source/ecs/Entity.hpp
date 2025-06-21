#pragma once
#include "Base/Platform.hpp"

namespace spite
{
	struct Entity
	{
	private:
		u64 m_id;

	public:
		explicit Entity(const u64 id = 0);
		u64 id() const;
		friend bool operator==(const Entity& lhs, const Entity& rhs);
		friend bool operator!=(const Entity& lhs, const Entity& rhs);

		struct hash
		{
			size_t operator()(const spite::Entity& entity) const;
		};

		static Entity undefined()
		{
			return Entity(0);
		}
	};

}
