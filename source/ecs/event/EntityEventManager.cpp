#include "EntityEventManager.hpp"
#include "ecs/core/EntityManager.hpp"
#include "ecs/query/QueryBuilder.hpp"

namespace spite
{
	EntityEventManager::EntityEventManager(EntityManager* entityManager)
		: m_entityManager(entityManager), m_commandBuffer(m_entityManager->createCommandBuffer())
	{
	}

	void EntityEventManager::commit()
	{
		m_commandBuffer.commit(*m_entityManager);
	}
}
