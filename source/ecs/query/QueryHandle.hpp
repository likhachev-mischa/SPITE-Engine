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
		QueryRegistry* m_queryRegistry{};
		QueryDescriptor m_descriptor{};
		mutable Query* m_cachedQuery = nullptr;

		// Fetches the fresh, potentially rebuilt query from the registry.
		Query* getQuery()
		{
			SASSERTM(m_queryRegistry, "QueryHandle was not initialized")
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
		QueryHandle() = default;

		QueryHandle(QueryRegistry* registry, const QueryDescriptor& desc)
			: m_queryRegistry(registry), m_descriptor(desc)
		{
		}

		QueryHandle(const QueryHandle& other) = default;
		QueryHandle(QueryHandle&& other) noexcept = default;
		QueryHandle& operator =(QueryHandle&& other) noexcept = default;
		QueryHandle& operator=(const QueryHandle& other) = default;

		bool isValid() const { return m_queryRegistry != nullptr; }

		// Expose the descriptor for advanced usage like prerequisite checks.
		const QueryDescriptor& getDescriptor() const { return m_descriptor; }

		// --- Forwarded Query API ---

		sizet getEntityCount() { return getQuery()->getEntityCount(); }

		void forEachConstChunk(const std::function<void(const Chunk* chunk)>& func) const
		{
			const Query* query = getQuery();
			return query->forEachChunk(func);
		}

		void forEachChunk(const std::function<void(Chunk* chunk)>& func)
		{
			return getQuery()->forEachChunk(func);
		}

		template <typename... TArgs>
		auto view() { return getQuery()->view<TArgs...>(); }

		template <typename... TArgs>
		auto view() const { return getQuery()->view<TArgs...>(); }

		template <typename... TArgs>
		auto begin() { return getQuery()->begin<TArgs...>(); }

		template <typename... TArgs>
		auto end() { return getQuery()->end<TArgs...>(); }
	};
}
