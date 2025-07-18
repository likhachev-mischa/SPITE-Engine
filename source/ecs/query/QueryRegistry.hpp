#pragma once

#include "ecs/storage/Aspect.hpp"
#include "ecs/query/Query.hpp"
#include "ecs/storage/VersionManager.hpp"

namespace spite
{
    class ArchetypeManager;
    class AspectRegistry;

    struct QueryDescriptor
    {
        const Aspect* includeAspect;
        const Aspect* excludeAspect;
        const Aspect* enabledAspect;
        const Aspect* modifiedAspect;

        bool operator==(const QueryDescriptor& other) const;

        struct hash
        {
            sizet operator()(const QueryDescriptor& desc) const;
        };
    };

    class QueryRegistry
    {
        ArchetypeManager* m_archetypeManager;
		VersionManager* m_versionManager;
        heap_unordered_map<QueryDescriptor, Query, QueryDescriptor::hash> m_queries;
    public:
        QueryRegistry(const HeapAllocator& allocator, ArchetypeManager* archetypeManager, VersionManager* versionManager);

        Query* findOrCreateQuery(const QueryDescriptor& descriptor);

        void rebuildAll();

    };
}
