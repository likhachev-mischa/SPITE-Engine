#include "Aspect.hpp"

#include "base/CollectionUtilities.hpp"

namespace spite
{
	Aspect::Aspect() = default;

	Aspect::Aspect(std::initializer_list<ComponentID> ids): m_componentIds(ids)
	{
		eastl::sort(m_componentIds.begin(), m_componentIds.end());

		// Manual unique removal
		auto writeIt = m_componentIds.begin();
		for (auto readIt = m_componentIds.begin(); readIt != m_componentIds.end(); ++readIt)
		{
			if (writeIt == m_componentIds.begin() || *readIt != *(writeIt - 1))
			{
				*writeIt = *readIt;
				++writeIt;
			}
		}
		while (m_componentIds.end() != writeIt)
		{
			m_componentIds.pop_back();
		}
	}

	Aspect::Aspect(ComponentID id)
	{
		m_componentIds.push_back(id);
	}

	Aspect::Aspect(eastl::span<const ComponentID> ids)
	{
		m_componentIds.reserve(ids.size());
		for (const auto& id : ids)
		{
			m_componentIds.push_back(id);
		}
		eastl::sort(m_componentIds.begin(), m_componentIds.end());
		// Manual unique removal
		auto writeIt = m_componentIds.begin();
		for (auto readIt = m_componentIds.begin(); readIt != m_componentIds.end(); ++readIt)
		{
			if (writeIt == m_componentIds.begin() || *readIt != *(writeIt - 1))
			{
				*writeIt = *readIt;
				++writeIt;
			}
		}
		while (m_componentIds.end() != writeIt)
		{
			m_componentIds.pop_back();
		}
	}

	Aspect::Aspect(const Aspect& other) = default;
	Aspect::Aspect(Aspect&& other) noexcept = default;
	Aspect& Aspect::operator=(const Aspect& other) = default;
	Aspect& Aspect::operator=(Aspect&& other) noexcept = default;

	Aspect Aspect::operator+(const Aspect& other) const
	{
		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto types = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
		types.reserve(other.size() + size());
		types.insert(types.end(), m_componentIds.begin(), m_componentIds.end());
		types.insert(types.end(), other.m_componentIds.begin(), other.m_componentIds.end());
		return Aspect(types.begin(), types.end());
	}

	bool Aspect::operator==(const Aspect& other) const
	{
		return m_componentIds == other.m_componentIds;
	}

	bool Aspect::operator!=(const Aspect& other) const
	{
		return !(*this == other);
	}

	bool Aspect::operator<(const Aspect& other) const
	{
		return m_componentIds < other.m_componentIds;
	}

	bool Aspect::operator>(const Aspect& other) const
	{
		return other < *this;
	}

	bool Aspect::operator<=(const Aspect& other) const
	{
		return other >= *this;
	}

	bool Aspect::operator>=(const Aspect& other) const
	{
		return !(*this < other);
	}

	const sbo_vector<ComponentID>& Aspect::getComponentIds() const
	{
		return m_componentIds;
	}

	Aspect Aspect::add(eastl::span<const ComponentID> ids) const
	{
		auto allocatorMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto tempIds = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
		for (const auto& id : ids)
		{
			tempIds.push_back(id);
		}
		for (const auto& id : getComponentIds())
		{
			tempIds.push_back(id);
		}
		return Aspect(tempIds);
	}

	Aspect Aspect::remove(eastl::span<const ComponentID> ids) const
	{
		auto allocatorMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto tempIds = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
		for (const auto& id : getComponentIds())
		{
			//if not removed
			if (std::ranges::find(ids, id) == ids.end())
			{
				tempIds.push_back(id);
			}
		}
		return Aspect(tempIds);
	}

	bool Aspect::contains(const Aspect& other) const
	{
		// Manual implementation of subset check for sorted vectors
		auto it1 = m_componentIds.begin();
		auto it2 = other.m_componentIds.begin();

		if (std::distance(it1, m_componentIds.end()) < std::distance(it2, other.m_componentIds.end()))
		{
			return false;
		}

		while (it1 != m_componentIds.end() && it2 != other.m_componentIds.end())
		{
			if (*it1 < *it2)
			{
				++it1;
			}
			else if (*it1 == *it2)
			{
				++it1;
				++it2;
			}
			else
			{
				// *it1 > *it2, meaning other has a type that this doesn't have
				return false;
			}
		}

		// If we've processed all of other's types, then this contains other
		return it2 == other.m_componentIds.end();
	}

	bool Aspect::contains(ComponentID id) const
	{
		return eastl::binary_search(m_componentIds.begin(), m_componentIds.end(), id);
	}

	bool Aspect::intersects(const Aspect& other) const
	{
		// Use two-pointer technique for sorted vectors
		auto it1 = m_componentIds.begin();
		auto it2 = other.m_componentIds.begin();

		while (it1 != m_componentIds.end() && it2 != other.m_componentIds.end())
		{
			if (*it1 == *it2)
			{
				return true;
			}
			if (*it1 < *it2)
			{
				++it1;
			}
			else
			{
				++it2;
			}
		}
		return false;
	}

	scratch_vector<ComponentID> Aspect::getIntersection(const Aspect& other) const
	{
		auto result = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
		auto it1 = m_componentIds.begin();
		auto it2 = other.m_componentIds.begin();

		while (it1 != m_componentIds.end() && it2 != other.m_componentIds.end())
		{
			if (*it1 == *it2)
			{
				result.push_back(*it1);
			}
			if (*it1 < *it2)
			{
				++it1;
			}
			else
			{
				++it2;
			}
		}
		return result;
	}

	sizet Aspect::size() const
	{
		return m_componentIds.size();
	}

	bool Aspect::empty() const
	{
		return m_componentIds.empty();
	}

	sizet Aspect::hash::operator()(const Aspect& aspect) const
	{
		size_t seed = 0;
		for (const auto& id : aspect.m_componentIds)
		{
			// Combine hashes using a simple hash combination method
			seed ^= std::hash<ComponentID>{}(id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}

	Aspect::~Aspect() = default;

}
