#pragma once
#include "EntityManager.hpp"
#include "ecs/generated/GeneratedSystemManager.hpp"
#include "ecs/query/QueryRegistry.hpp"
#include "ecs/generated/GeneratedSystemManager.hpp"

namespace spite
{
	class EntityWorld
	{
	private:
		HeapAllocator m_allocator;

		ComponentMetadataRegistry m_componentMetadataRegistry;
		AspectRegistry m_aspectRegistry;
		VersionManager m_versionManager;

		SharedComponentManager m_sharedComponentManager;
		SharedComponentRegistryBridge m_sharedComponentRegistryBridge;
		ComponentMetadataInitializer m_componentMetadataInitializer;

		ArchetypeManager m_archetypeManager;
		QueryRegistry m_queryRegistry;

		EntityManager m_entityManager;
		SystemManager m_systemManager;

	public:
		EntityWorld(const HeapAllocator& worldAllocator) :
			m_allocator(worldAllocator),
			m_aspectRegistry(m_allocator),
			m_versionManager(m_allocator, &m_aspectRegistry),
			m_sharedComponentManager(m_componentMetadataRegistry, m_allocator),
			m_sharedComponentRegistryBridge(&m_sharedComponentManager),
			m_componentMetadataInitializer(&m_componentMetadataRegistry, &m_sharedComponentRegistryBridge),

			m_archetypeManager(&m_componentMetadataRegistry, m_allocator, &m_aspectRegistry, &m_versionManager),
			m_queryRegistry(m_allocator, &m_archetypeManager, &m_versionManager,
			                &m_aspectRegistry, &m_componentMetadataRegistry),

			m_entityManager(m_archetypeManager, m_sharedComponentManager, m_componentMetadataRegistry,
			                &m_aspectRegistry, &m_queryRegistry),
			m_systemManager(&m_entityManager)
		{
		}

		EntityManager& getEntityManager() { return m_entityManager; }

		void update(float deltaTime)
		{
			m_systemManager.update(deltaTime);
		}
	};
}
