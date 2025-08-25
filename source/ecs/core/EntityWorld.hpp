#pragma once
#include "EntityManager.hpp"
#include "ecs/systems/SystemManager.hpp"
#include "ecs/query/QueryRegistry.hpp"
#include "ecs/storage/AspectRegistry.hpp"
#include "ecs/core/ComponentMetadataRegistry.hpp"

namespace spite
{
	class EntityWorld
	{
	private:
		HeapAllocator m_allocator;

		AspectRegistry m_aspectRegistry;
		VersionManager m_versionManager;

		SharedComponentManager m_sharedComponentManager;

		ArchetypeManager m_archetypeManager;
		QueryRegistry m_queryRegistry;

		SingletonComponentRegistry m_singletonComponentRegistry;

		EntityManager m_entityManager;
		SystemManager m_systemManager;

	public:
		EntityWorld(const HeapAllocator& worldAllocator) :
			m_allocator(worldAllocator),
			m_aspectRegistry(m_allocator),
			m_versionManager(m_allocator, &m_aspectRegistry),
			m_sharedComponentManager(m_allocator),
			m_archetypeManager(m_allocator, &m_aspectRegistry, &m_versionManager, &m_sharedComponentManager),
			m_queryRegistry(m_allocator, &m_archetypeManager, &m_versionManager),
			m_singletonComponentRegistry(m_allocator),
			m_entityManager(&m_archetypeManager, &m_sharedComponentManager, &m_singletonComponentRegistry,
			                &m_aspectRegistry, &m_queryRegistry, m_allocator),
			m_systemManager(m_allocator, &m_entityManager, &m_versionManager)
		{
		}

		SystemManager& getSystemManager() { return m_systemManager; }
		EntityManager& getEntityManager() { return m_entityManager; }

		void initialize()
		{
			m_systemManager.initialize();
		}

		void update(float deltaTime)
		{
			m_systemManager.update(deltaTime);
		}
	};
}
