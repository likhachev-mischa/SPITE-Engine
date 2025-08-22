#pragma once
#include "IEventComponent.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"

namespace spite
{
	class EntityEventManager
	{
	private:
		EntityManager* m_entityManager;
		CommandBuffer m_commandBuffer;

	public:
		EntityEventManager(EntityManager* entityManager);

		template <t_event T>
		void fire(T&& event = T{});

		//call right in the start of the frame
		void commit();
	};

	template <t_event T>
	void EntityEventManager::fire(T&& event)
	{

		Entity e = m_commandBuffer.createEntity();
		m_commandBuffer.addComponent(e, EventTag{});
		m_commandBuffer.addComponent<T>(e, std::forward<T>(event));
	}
}
