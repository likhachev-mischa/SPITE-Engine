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
			                                         descriptor.readAspect, descriptor.writeAspect,
			                                         descriptor.excludeAspect, descriptor.enabledAspect,
			                                         descriptor.modifiedAspect)).first;
			//SDEBUG_LOG("Creating new query (include aspect: %p).\n",
			//(void*)descriptor.includeAspect)
		}

		Query& query = it->second;
		const u64 currentVersion = m_versionManager->getVersion(*descriptor.includeAspect);

		if (query.m_includeVersion != currentVersion)
		{
			//SDEBUG_LOG("Rebuilding query (include aspect: %p) because version changed from %llu to %llu.\n",
			//          (void*)descriptor.includeAspect, query.m_includeVersion, currentVersion)
			query.rebuild(*m_archetypeManager);
			query.m_includeVersion = currentVersion;
		}

		return &query;
	}

	void QueryRegistry::rebuildAll()
	{
		for (auto& [descriptor, query] : m_queries)
		{
			auto marker = FrameScratchAllocator::get().get_scoped_marker();
			auto allIds = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
			const auto& readIds = descriptor.readAspect->getComponentIds();
			const auto& writeIds = descriptor.writeAspect->getComponentIds();
			allIds.reserve(readIds.size() + writeIds.size());
			allIds.insert(allIds.end(), readIds.begin(), readIds.end());
			allIds.insert(allIds.end(), writeIds.begin(), writeIds.end());

			Aspect includeAspect(allIds.begin(), allIds.end());

			query.rebuild(*m_archetypeManager);
			query.m_includeVersion = m_versionManager->getVersion(includeAspect);
		}
	}

	bool QueryDescriptor::operator==(const QueryDescriptor& other) const
	{
		return includeAspect == other.includeAspect &&
			readAspect == other.readAspect &&
			writeAspect == other.writeAspect &&
			excludeAspect == other.excludeAspect &&
			enabledAspect == other.enabledAspect &&
			modifiedAspect == other.modifiedAspect;
	}

	sizet QueryDescriptor::hash::operator()(const QueryDescriptor& desc) const
	{
		sizet seed = 0;
		eastl::hash<const Aspect*> hasher;
		seed ^= hasher(desc.includeAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hasher(desc.readAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hasher(desc.writeAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hasher(desc.excludeAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hasher(desc.enabledAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hasher(desc.modifiedAspect) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
}
