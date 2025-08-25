#include "EntityManager.hpp"

#include "ecs/cbuffer/CommandBuffer.hpp"
#include "ecs/query/QueryBuilder.hpp"

namespace spite
{
	EntityManager::EntityManager(ArchetypeManager* archetypeManager, SharedComponentManager* sharedComponentManager,
	                             SingletonComponentRegistry* singletonComponentRegistry, AspectRegistry* aspectRegistry,
	                             QueryRegistry* queryRegistry, const HeapAllocator& allocator)
		: m_archetypeManager(archetypeManager),
		  m_sharedComponentManager(sharedComponentManager),
		  m_aspectRegistry(aspectRegistry),
		  m_singletonComponentRegistry(
			  singletonComponentRegistry),
		  m_queryRegistry(queryRegistry),
		  m_eventManager(this),
		  m_generations(makeHeapVector<u32>(allocator)), m_freeIndices(makeHeapVector<u32>(allocator)),
		  m_allocator(allocator)
	{
		m_generations.push_back(0);
	}

	QueryBuilder EntityManager::getQueryBuilder() const
	{
		return QueryBuilder(m_queryRegistry, m_aspectRegistry);
	}

	CommandBuffer EntityManager::createCommandBuffer() const
	{
		return CommandBuffer(m_archetypeManager, m_allocator);
	}

	Entity EntityManager::createEntity(const Aspect& aspect)
	{
		u32 index;
		if (!m_freeIndices.empty())
		{
			index = m_freeIndices.back();
			m_freeIndices.pop_back();
		}
		else
		{
			index = static_cast<u32>(m_generations.size());
			m_generations.push_back(0);
		}
		Entity entity(index, m_generations[index]);
		m_archetypeManager->addEntity(aspect, entity);
		return entity;
	}

	EntityEventManager& EntityManager::getEventManager()
	{
		return m_eventManager;
	}

	void EntityManager::destroyEntity(Entity entity)
	{
		SASSERT(isValid(entity))

		m_archetypeManager->removeEntity(entity);

		const u32 index = entity.index();
		++m_generations[index];
		m_freeIndices.push_back(index);
	}

	void EntityManager::destroyEntities(eastl::span<const Entity> entities)
	{
		for (const auto& entity : entities)
		{
			destroyEntity(entity);
		}
	}

	bool EntityManager::isValid(Entity entity) const
	{
		const u32 index = entity.index();
		return (entity != Entity::undefined()) && (index < m_generations.size() && m_generations[index] == entity.
			generation());
	}

	void EntityManager::addComponents(eastl::span<const Entity> entities,
	                                  eastl::span<const ComponentID> componentIds) const
	{
		for (const auto& entity : entities)
			SASSERT(isValid(entity))
		m_archetypeManager->addComponents(entities, componentIds);
	}

	void EntityManager::moveEntities(const Aspect& toAspect, eastl::span<const Entity> entities) const
	{
		for (const auto& entity : entities)
			SASSERT(isValid(entity))
		m_archetypeManager->moveEntities(toAspect, entities);
	}

	void EntityManager::removeComponents(eastl::span<const Entity> entities,
	                                     eastl::span<ComponentID> componentIds) const
	{
		for (const auto& entity : entities)
			SASSERT(isValid(entity))
		m_archetypeManager->removeComponents(entities, componentIds);
	}

	bool EntityManager::hasComponent(Entity entity, ComponentID id) const
	{
		SASSERT(isValid(entity))
		const Archetype& archetype = m_archetypeManager->getEntityArchetype(entity);
		return archetype.aspect().contains(id);
	}

	void EntityManager::setComponentData(Entity entity, ComponentID componentId, void* componentData) const
	{
		SASSERT(isValid(entity))
		const Archetype& archetype = m_archetypeManager->getEntityArchetype(entity);
		auto [chunk, indexInChunk] = archetype.getEntityLocation(entity);

		const int componentIndexInChunk = archetype.getComponentIndex(componentId);
		SASSERTM(componentIndexInChunk != -1, "Entity %llu has no component %u for setComponentData\n", entity.id(),
		         componentId)
		void* dest = chunk->getComponentDataPtrByIndex(componentIndexInChunk, indexInChunk);

		ComponentMetadataRegistry::getMetadata(componentId).moveAndDestroy(dest, componentData);
	}
}
