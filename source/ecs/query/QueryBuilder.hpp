#pragma once

#include "base/CollectionAliases.hpp"
#include "ecs/core/ComponentMetadataRegistry.hpp"
#include "ecs/query/QueryHandle.hpp"

namespace spite
{
	class AspectRegistry;
	class QueryRegistry;

	class QueryBuilder
	{
	private:
		QueryRegistry* m_queryRegistry;
		AspectRegistry* m_aspectRegistry;
		scratch_vector<ComponentID> m_includeTypes;
		scratch_vector<ComponentID> m_excludeTypes;
		scratch_vector<ComponentID> m_enabledTypes;
		scratch_vector<ComponentID> m_modifiedTypes;

	public:
		QueryBuilder(QueryRegistry* queryRegistry, AspectRegistry* aspectRegistry);

		template <t_component... T>
		QueryBuilder& with()
		{
			(m_includeTypes.push_back(ComponentMetadataRegistry::getComponentId<T>()), ...);
			return *this;
		}

		template <t_component... T>
		QueryBuilder& without()
		{
			(m_excludeTypes.push_back(ComponentMetadataRegistry::getComponentId<T>()), ...);
			return *this;
		}

		template <t_component... T>
		QueryBuilder& enabled()
		{
			(m_enabledTypes.push_back(ComponentMetadataRegistry::getComponentId<T>()), ...);
			return *this;
		}

		template <t_component... T>
		QueryBuilder& modified()
		{
			(m_modifiedTypes.push_back(ComponentMetadataRegistry::getComponentId<T>()), ...);
			return *this;
		}

		QueryHandle build();
	};
}
