#pragma once
#include "ecs/EntityManager.hpp"

namespace spite
{
	class EntityObserver
	{
		EntityManager* m_entityManager;
		ComponentStorage* m_componentStorage;
		ComponentLookup* m_componentLookup;

		void handler(Entity entity)
		{
			m_componentLookup->trackEntity(entity);
		}
	public:
		EntityObserver(EntityManager* entityManager, ComponentStorage* componentStorage,
		               ComponentLookup* componentLookup)
			: m_entityManager(entityManager),
			  m_componentStorage(componentStorage),
			  m_componentLookup(componentLookup)
		{
			m_entityManager->addOnEntityCreatedHandler([this](Entity entity) { this->handler(entity); });
		}

	};
}
