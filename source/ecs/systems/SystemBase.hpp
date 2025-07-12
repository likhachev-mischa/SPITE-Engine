#pragma once
#include "ecs/core/EntityManager.hpp"

namespace spite
{
	class SystemBase
	{
	protected:
		EntityManager* m_entityManager;

	public:
		bool isActive = true;

		SystemBase(EntityManager* entityManager): m_entityManager(entityManager)
		{
		}

		virtual void onInitialize(){};
		virtual void onUpdate(float deltaTime){};

		virtual void onDestroy(){};
		virtual ~SystemBase() = default;
	};
}
