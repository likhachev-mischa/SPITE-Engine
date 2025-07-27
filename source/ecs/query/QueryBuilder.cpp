#include "QueryBuilder.hpp"

#include "QueryRegistry.hpp"

#include "ecs/storage/AspectRegistry.hpp"

namespace spite
{
	QueryBuilder::QueryBuilder(QueryRegistry* queryRegistry, AspectRegistry* aspectRegistry) :
		m_queryRegistry(queryRegistry), m_aspectRegistry(aspectRegistry),
		m_readTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get())),
		m_writeTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get())),
		m_excludeTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get())),
		m_enabledTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get())),
		m_modifiedTypes(makeScratchVector<ComponentID>(FrameScratchAllocator::get()))
	{
	}

	QueryHandle QueryBuilder::build()
	{
		//the included aspect should contain read, write, enabled and modified types all together
		auto allocMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto allTypes = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
		allTypes.insert(allTypes.end(), m_readTypes.begin(), m_readTypes.end());
		allTypes.insert(allTypes.end(), m_writeTypes.begin(), m_writeTypes.end());
		allTypes.insert(allTypes.end(), m_enabledTypes.begin(), m_enabledTypes.end());
		allTypes.insert(allTypes.end(), m_modifiedTypes.begin(), m_modifiedTypes.end());

		const Aspect* readAspect = m_aspectRegistry->addOrGetAspect(Aspect(m_readTypes.begin(), m_readTypes.end()));
		const Aspect* writeAspect = m_aspectRegistry->addOrGetAspect(Aspect(m_writeTypes.begin(), m_writeTypes.end()));
		const Aspect* includeAspect = m_aspectRegistry->addOrGetAspect(Aspect(allTypes.begin(), allTypes.end()));
		QueryDescriptor descriptor = {
			.includeAspect = includeAspect,
			.readAspect = readAspect,
			.writeAspect = writeAspect,
			.excludeAspect = m_aspectRegistry->addOrGetAspect(
				Aspect(m_excludeTypes.begin(), m_excludeTypes.end())),
			.enabledAspect = m_aspectRegistry->addOrGetAspect(
				Aspect(m_enabledTypes.begin(), m_enabledTypes.end())),
			.modifiedAspect = m_aspectRegistry->addOrGetAspect(
				Aspect(m_modifiedTypes.begin(), m_modifiedTypes.end()))
		};
		return QueryHandle(m_queryRegistry, descriptor);
	}
}
