#pragma once

#include <EASTL/sort.h>
#include <EASTL/span.h>

#include "base/CollectionAliases.hpp"

#include "ecs/core/ComponentMetadata.hpp"

namespace spite
{
	class Aspect
	{
	private:
		sbo_vector<ComponentID> m_componentIds;

	public:
		Aspect();

		Aspect(std::initializer_list<ComponentID> ids);

		template <typename Iterator>
		Aspect(Iterator begin, Iterator end) requires std::is_base_of_v<
			std::forward_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>;

		explicit Aspect(ComponentID id);

		explicit Aspect(eastl::span<const ComponentID> ids);

		Aspect(const Aspect& other);
		Aspect(Aspect&& other) noexcept;
		Aspect& operator=(const Aspect& other);
		Aspect& operator=(Aspect&& other) noexcept;

		bool operator==(const Aspect& other) const;

		bool operator!=(const Aspect& other) const;

		bool operator<(const Aspect& other) const;

		bool operator>(const Aspect& other) const;

		bool operator<=(const Aspect& other) const;

		bool operator>=(const Aspect& other) const;

		const sbo_vector<ComponentID>& getComponentIds() const;

		//returns new aspect
		Aspect add(eastl::span<const ComponentID> ids) const;

		//returns new aspect
		Aspect remove(eastl::span<const ComponentID> ids) const;

		// Check if this aspect contains all types from another aspect (subset check)
		bool contains(const Aspect& other) const;

		bool contains(ComponentID id) const;

		// Check if this aspect intersects with another (has common types)
		bool intersects(const Aspect& other) const;

		// get all common types
		scratch_vector<ComponentID> getIntersection(const Aspect& other) const;

		sizet size() const;

		bool empty() const;

		struct hash
		{
			sizet operator()(const Aspect& aspect) const;
		};

		~Aspect();
	};

	template <typename Iterator>
	Aspect::Aspect(Iterator begin, Iterator end) requires std::is_base_of_v<
		std::forward_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>
	{
		sizet dist = eastl::distance(begin, end);
		if (dist <= 0) return;

		m_componentIds.reserve(dist);
		for (auto it = begin; it != end; ++it)
		{
			m_componentIds.emplace_back(*it);
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
}
