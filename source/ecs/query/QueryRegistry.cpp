#include "QueryRegistry.hpp"
#include "ecs/storage/Archetype.hpp"

namespace spite
{
	QueryRegistry::QueryRegistry(const HeapAllocator& allocator, ArchetypeManager* archetypeManager,
	                             VersionManager* versionManager)
		: m_archetypeManager(archetypeManager),
		  m_versionManager(versionManager),
		  m_queries(makeHeapMap<QueryDescriptor, Query, QueryDescriptor::hash>(allocator))
	{
	}

	Query* QueryRegistry::findOrCreateQuery(const QueryDescriptor& descriptor)
	{
		auto it = m_queries.find(descriptor);
		if (it == m_queries.end())
		{
			it = m_queries.emplace(descriptor, Query(m_archetypeManager, descriptor.includeAspect,
			                                         descriptor.excludeAspect, descriptor.enabledAspect,
			                                         descriptor.modifiedAspect)).first;
		}

		Query& query = it->second;

		const u64 currentVersion = m_versionManager->getVersion(*descriptor.includeAspect);

		if (query.m_includeVersion != currentVersion)
		{
			query.rebuild(*m_archetypeManager);
			query.m_includeVersion = currentVersion;
		}

		return &query;
	}

	void QueryRegistry::rebuildAll()
	{
		for (auto& [descriptor, query] : m_queries)
		{
			query.rebuild(*m_archetypeManager);
			query.m_includeVersion = m_versionManager->getVersion(*descriptor.includeAspect);
		}
	}

	bool QueryDescriptor::operator==(const QueryDescriptor& other) const
	{
		return includeAspect == other.includeAspect &&
			excludeAspect == other.excludeAspect &&
			enabledAspect == other.enabledAspect &&
			modifiedAspect == other.modifiedAspect;
	}

	sizet QueryDescriptor::hash::operator()(const QueryDescriptor& desc) const
	{
		sizet seed = 0;
		eastl::hash<const Aspect*> hasher;
		seed ^= hasher(desc.includeAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hasher(desc.excludeAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hasher(desc.enabledAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hasher(desc.modifiedAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
}
