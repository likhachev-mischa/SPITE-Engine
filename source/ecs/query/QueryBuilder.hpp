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
		scratch_vector<ComponentID> m_readTypes;
		scratch_vector<ComponentID> m_writeTypes;
		scratch_vector<ComponentID> m_excludeTypes;
		scratch_vector<ComponentID> m_enabledTypes;
		scratch_vector<ComponentID> m_modifiedTypes;

	public:
		QueryBuilder(QueryRegistry* queryRegistry, AspectRegistry* aspectRegistry);

		template <typename... T>
		QueryBuilder& with()
		{
			(
				[&]<typename U>(std::type_identity<U>)
				{
					static_assert(is_read_wrapper_v<U> || is_write_wrapper_v<U>,
					              "with<>() must be called with Read<T> or Write<T> wrappers.");
					using ComponentType = get_component_type<U>;
					if constexpr (is_read_wrapper_v<U>)
					{
						m_readTypes.push_back(ComponentMetadataRegistry::getComponentId<ComponentType>());
					}
					else
					{
						m_writeTypes.push_back(ComponentMetadataRegistry::getComponentId<ComponentType>());
					}
				}(std::type_identity<T>{}),
				...);
			return *this;
		}

		template <t_component... T>
		QueryBuilder& with_read()
		{
			(m_readTypes.push_back(ComponentMetadataRegistry::getComponentId<T>()), ...);
			return *this;
		}

		template <t_component... T>
		QueryBuilder& with_write()
		{
			(m_writeTypes.push_back(ComponentMetadataRegistry::getComponentId<T>()), ...);
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
