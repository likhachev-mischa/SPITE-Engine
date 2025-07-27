#include "EventManager.hpp"
#include "ecs/core/EntityManager.hpp"
#include "ecs/query/QueryBuilder.hpp"

namespace spite
{
	EventManager::EventManager(EntityManager* entityManager)
		: m_entityManager(entityManager), m_commandBuffer(m_entityManager->getCommandBuffer())
	{
	}

	void EventManager::commit()
	{
		m_commandBuffer.commit(*m_entityManager);
	}

	void EventManager::cleanup()
	{
		auto query = m_entityManager->getQueryBuilder().with<Read<EventTag>>().build();

		for (auto [tag,entity] : query.view<Read<EventTag>, Entity>())
		{
			m_commandBuffer.destroyEntity(entity);
		}

		m_commandBuffer.commit(*m_entityManager);
	}
}
