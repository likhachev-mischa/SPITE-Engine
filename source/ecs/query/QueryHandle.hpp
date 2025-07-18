#pragma once

#include "ecs/query/Query.hpp"
#include "ecs/query/QueryRegistry.hpp"

namespace spite
{
	// A lightweight handle to a query that is safe to cache in systems.
	// It automatically handles fetching the up-to-date query from the registry,
	// ensuring that systems always iterate over the correct set of archetypes
	// without needing to manually refresh the query pointer every frame.
	class QueryHandle
	{
	private:
		QueryRegistry* m_queryRegistry;
		QueryDescriptor m_descriptor;
		mutable Query* m_cachedQuery = nullptr;

		// Fetches the fresh, potentially rebuilt query from the registry.
		Query* getQuery()
		{
			// This single call performs the lazy-rebuild check.
			// The result is cached for the duration of the frame's operations.
			m_cachedQuery = m_queryRegistry->findOrCreateQuery(m_descriptor);
			return m_cachedQuery;
		}

		// Const version for const methods.
		const Query* getQuery() const
		{
			m_cachedQuery = m_queryRegistry->findOrCreateQuery(m_descriptor);
			return m_cachedQuery;
		}

	public:
		QueryHandle(QueryRegistry* registry, const QueryDescriptor& desc)
			: m_queryRegistry(registry), m_descriptor(desc)
		{
		}

		// Expose the descriptor for advanced usage like prerequisite checks.
		const QueryDescriptor& getDescriptor() const { return m_descriptor; }

		// --- Forwarded Query API ---

		sizet getEntityCount() { return getQuery()->getEntityCount(); }
		bool wasModified() const { return getQuery()->wasModified(); }
		void resetModificationStatus() const { getQuery()->resetModificationStatus(); }

		template <typename TFunc>
		void forEachChunk(TFunc& func) { getQuery()->forEachChunk(func); }

		// --- Views (most common usage) ---

		template <typename... TArgs>
		auto view() { return getQuery()->view<TArgs...>(); }

		template <typename... TArgs>
		auto view() const { return getQuery()->view<TArgs...>(); }

		template <typename... TArgs>
		auto cview() const { return getQuery()->cview<TArgs...>(); }

		template <typename... TArgs>
		auto enabled_view() { return getQuery()->enabled_view<TArgs...>(); }

		template <typename... TArgs>
		auto cenabled_view() const { return getQuery()->cenabled_view<TArgs...>(); }

		template <typename... TArgs>
		auto modified_view() { return getQuery()->modified_view<TArgs...>(); }

		template <typename... TArgs>
		auto cmodified_view() const { return getQuery()->cmodified_view<TArgs...>(); }

		template <typename... TArgs>
		auto enabled_modified_view() { return getQuery()->enabled_modified_view<TArgs...>(); }

		template <typename... TArgs>
		auto cenabled_modified_view() const { return getQuery()->cenabled_modified_view<TArgs...>(); }

		// --- Iterators (less common, but supported) ---

		template <typename... TArgs>
		auto begin() { return getQuery()->begin<TArgs...>(); }

		template <typename... TArgs>
		auto end() { return getQuery()->end<TArgs...>(); }

		template <typename... TArgs>
		auto cbegin() const { return getQuery()->cbegin<TArgs...>(); }

		template <typename... TArgs>
		auto cend() const { return getQuery()->cend<TArgs...>(); }
	};
}
