#include "QueryBuilder.hpp"

#include "ecs/query/Query.hpp"

namespace spite
{
	QueryBuilder::QueryBuilder(QueryRegistry& queryRegistry, AspectRegistry& aspectRegistry,const ComponentMetadataRegistry& metadataRegistry) :
		m_queryRegistry(queryRegistry), m_aspectRegistry(aspectRegistry),m_metadataRegistry(metadataRegistry),
		m_includeTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get())),
		m_excludeTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get())),
		m_enabledTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get())),
		m_modifiedTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get()))
	{
	}

	Query* QueryBuilder::build()
	{
		QueryDescriptor descriptor = {
			.includeAspect = m_aspectRegistry.addOrGetAspect(
				Aspect(m_includeTypes.begin(), m_includeTypes.end())),
			.excludeAspect = m_aspectRegistry.addOrGetAspect(
				Aspect(m_excludeTypes.begin(), m_excludeTypes.end())),
			.enabledAspect = m_aspectRegistry.addOrGetAspect(
				Aspect(m_enabledTypes.begin(), m_enabledTypes.end())),
			.modifiedAspect = m_aspectRegistry.addOrGetAspect(
				Aspect(m_modifiedTypes.begin(), m_modifiedTypes.end()))
		};
		return m_queryRegistry.findOrCreateQuery(descriptor);
	}
}
