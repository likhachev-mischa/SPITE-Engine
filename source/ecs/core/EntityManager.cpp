#include "EntityManager.hpp"
#include "ComponentMetadata.hpp"
#include "ecs/query/QueryBuilder.hpp"

namespace spite
{
	EntityManager::EntityManager(ArchetypeManager& archetypeManager, SharedComponentManager& sharedComponentManager,
		const ComponentMetadataRegistry& metadataRegistry, AspectRegistry* aspectRegistry, QueryRegistry* queryRegistry): m_archetypeManager(archetypeManager),
		m_sharedComponentManager(sharedComponentManager),
		m_metadataRegistry(metadataRegistry), m_aspectRegistry(aspectRegistry), m_queryRegistry(queryRegistry),
		m_nextEntityId(1)
	{
	}

	QueryBuilder EntityManager::getQueryBuilder() const
	{
		return QueryBuilder(*m_queryRegistry, *m_aspectRegistry, m_metadataRegistry);
	}

	bool EntityManager::hasComponent(Entity entity, ComponentID id) const
	{
		const Archetype& archetype = m_archetypeManager.getEntityArchetype(entity);
		return archetype.aspect().contains(id);
	}

	void EntityManager::setComponentData(Entity entity, ComponentID componentId, void* componentData) const
	{
		const Archetype& archetype = m_archetypeManager.getEntityArchetype(entity);
		auto [chunk, indexInChunk] = archetype.getEntityLocation(entity);

		const int componentIndexInChunk = archetype.getComponentIndex(componentId);
		SASSERTM(componentIndexInChunk != -1, "Entity %llu has no component %u for setComponentData\n", entity.id(),
		         componentId)
		void* dest = chunk->getComponentDataPtrByIndex(componentIndexInChunk, indexInChunk);

		const auto& metadata = m_metadataRegistry.getMetadata(componentId);

		if (metadata.isTriviallyRelocatable)
		{
			memcpy(dest, componentData, metadata.size);
		}
		else
		{
			metadata.moveAndDestroy(dest, componentData);
		}
	}
}
