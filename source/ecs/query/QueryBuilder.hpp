#pragma once

#include "ecs/core/ComponentMetadataRegistry.hpp"

#include "ecs/storage/Aspect.hpp"
#include "ecs/query/QueryRegistry.hpp"

namespace spite
{
    class Query;

    class QueryBuilder
    {
    private:
        QueryRegistry& m_queryRegistry;
        AspectRegistry& m_aspectRegistry;
        const ComponentMetadataRegistry& m_metadataRegistry;
        scratch_vector<ComponentID> m_includeTypes;
        scratch_vector<ComponentID> m_excludeTypes;
        scratch_vector<ComponentID> m_enabledTypes;
        scratch_vector<ComponentID> m_modifiedTypes;
    public:
        QueryBuilder(QueryRegistry& queryRegistry, AspectRegistry& aspectRegistry,const ComponentMetadataRegistry& metadataRegistry);

        template <t_component... T>
        QueryBuilder& with()
        {
            (m_includeTypes.push_back(m_metadataRegistry.getComponentId(typeid(T))), ...);
            return *this;
        }

        template <t_component... T>
        QueryBuilder& without()
        {
            (m_excludeTypes.push_back(m_metadataRegistry.getComponentId(typeid(T))), ...);
            return *this;
        }

        template <t_component... T>
        QueryBuilder& enabled()
        {
            (m_enabledTypes.push_back(m_metadataRegistry.getComponentId(typeid(T))), ...);
            return *this;
        }

        template <t_component... T>
        QueryBuilder& modified()
        {
            (m_modifiedTypes.push_back(m_metadataRegistry.getComponentId(typeid(T))), ...);
            return *this;
        }

        Query* build();
    };
}
