#include "EntityManager.hpp"
#include "ecs/query/QueryBuilder.hpp"

namespace spite
{
	EntityManager::EntityManager(ArchetypeManager* archetypeManager, SharedComponentManager* sharedComponentManager,
	                             SingletonComponentRegistry* singletonComponentRegistry, AspectRegistry* aspectRegistry,
	                             QueryRegistry* queryRegistry): m_archetypeManager(archetypeManager),
	                                                            m_sharedComponentManager(sharedComponentManager),
	                                                            m_aspectRegistry(aspectRegistry),
	                                                            m_singletonComponentRegistry(
		                                                            singletonComponentRegistry),
	                                                            m_queryRegistry(queryRegistry),
	                                                            m_nextEntityId(1)
	{
	}

	QueryBuilder EntityManager::getQueryBuilder() const
	{
		return QueryBuilder(m_queryRegistry, m_aspectRegistry);
	}

	Entity EntityManager::createEntity(const Aspect& aspect)
	{
		Entity entity(m_nextEntityId++);
		m_archetypeManager->addEntity(aspect, entity);
		return entity;
	}

	void EntityManager::destroyEntity(Entity entity) const
	{
		m_archetypeManager->removeEntity(entity);
	}

	void EntityManager::destroyEntities(eastl::span<const Entity> entities) const
	{
		m_archetypeManager->removeEntities(entities);
	}

	void EntityManager::addComponents(eastl::span<const Entity> entities, eastl::span<const ComponentID> componentIds) const
	{
		m_archetypeManager->addComponents(entities, componentIds);
	}

	void EntityManager::moveEntities(const Aspect& toAspect, eastl::span<const Entity> entities) const
	{
		m_archetypeManager->moveEntities(toAspect, entities);
	}

	void EntityManager::removeComponents(eastl::span<const Entity> entities,
		eastl::span<ComponentID> componentIds) const
	{
		m_archetypeManager->removeComponents(entities, componentIds);
	}

	bool EntityManager::hasComponent(Entity entity, ComponentID id) const
	{
		const Archetype& archetype = m_archetypeManager->getEntityArchetype(entity);
		return archetype.aspect().contains(id);
	}

	void EntityManager::setComponentData(Entity entity, ComponentID componentId, void* componentData) const
	{
		const Archetype& archetype = m_archetypeManager->getEntityArchetype(entity);
		auto [chunk, indexInChunk] = archetype.getEntityLocation(entity);

		const int componentIndexInChunk = archetype.getComponentIndex(componentId);
		SASSERTM(componentIndexInChunk != -1, "Entity %llu has no component %u for setComponentData\n", entity.id(),
		         componentId)
		void* dest = chunk->getComponentDataPtrByIndex(componentIndexInChunk, indexInChunk);

		ComponentMetadataRegistry::getMetadata(componentId).moveAndDestroy(dest, componentData);
	}
}
