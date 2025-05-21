#pragma once
#include "application/input/InputManager.hpp"
#include "application/input/Keycodes.hpp"

#include "ecs/Queries.hpp"
#include "ecs/World.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/VulkanComponents.hpp"

namespace spite
{
	class ControllableEntitySelectSystem : public SystemBase
	{
		const cstring m_inputActionName = "SelectEntity";
		Query1<TransformComponent>* m_entityQuery;

	public:
		void onInitialize() override
		{
			m_entityQuery = m_entityService->queryBuilder()->buildQuery<TransformComponent>();
		}

		void onUpdate(float deltaTime) override
		{
			auto actionMap = m_entityService->componentManager()->getSingleton<
				InputManagerComponent>().inputManager->inputActionMap();
			if (!actionMap->isActionActive(m_inputActionName) || actionMap->actionValue(
				m_inputActionName) <= 0.5f)
			{
				return;
			}

			auto& controllableEntity = m_entityService->componentManager()->getSingleton<
				ControllableEntitySingleton>();
			Entity currentEntity = controllableEntity.entity;
			Entity newEntity;
			auto& entityQuery = *m_entityQuery;
			for (sizet i = 0, size = entityQuery.size(); i < size; ++i)
			{
				newEntity = entityQuery.owner(i);
				if (newEntity.id() > currentEntity.id())
				{
					SDEBUG_LOG("New entity %llu selected\n", newEntity.id());
					controllableEntity.entity = newEntity;
					return;
				}
			}

			newEntity = entityQuery.owner(0);
			SDEBUG_LOG("New entity %llu selected (from query start)\n", newEntity.id());
			controllableEntity.entity = newEntity;
		}
	};

	class MovementDirectionControlSystem : public SystemBase
	{
		const cstring m_yAxis = "YAxis";
		const cstring m_xAxis = "XAxis";

	public:
		void onUpdate(float deltaTime) override
		{
			auto actionMap = m_entityService->componentManager()->getSingleton<
				InputManagerComponent>().inputManager->inputActionMap();

			if (!actionMap->isActionActive(m_yAxis) && !actionMap->isActionActive(m_xAxis))
			{
				return;
			}

			auto targetEntity = m_entityService->componentManager()->getSingleton<
				ControllableEntitySingleton>().entity;
			auto& moveDirection = m_entityService->componentManager()->getComponent<
				MovementDirectionComponent>(targetEntity);

			float x = actionMap->actionValue(m_xAxis);
			float y = actionMap->actionValue(m_yAxis);

			moveDirection.direction[0] = x;
			moveDirection.direction[1] = 0;
			moveDirection.direction[2] = -y;
			if (x != 0.0f && y != 0.0f) moveDirection.direction = glm::normalize(
				moveDirection.direction);
			//SDEBUG_LOG("MOVED ON %f x %f y\n", moveDirection.direction[0], moveDirection.direction[1]);
		}
	};

	class RotationControlSystem : public SystemBase
	{
		const float m_rotationSensitivity = 0.005f;

	public:
		void onUpdate(float deltaTime) override
		{
			auto inputManager = m_entityService->componentManager()->getSingleton<
				InputManagerComponent>().inputManager;

			auto targetEntity = m_entityService->componentManager()->getSingleton<
				ControllableEntitySingleton>().entity;
			auto& transform = m_entityService->componentManager()->getComponent<TransformComponent>(
				targetEntity);

			const auto& mousePos = inputManager->mousePosition();
			float mouseX = mousePos.xRel * m_rotationSensitivity;
			float mouseY = mousePos.yRel * m_rotationSensitivity;

			glm::quat pitchQuat = glm::angleAxis(-mouseY, glm::vec3(1.0f, 0.0f, 0.0f));
			glm::quat yawQuat = glm::angleAxis(-mouseX, glm::vec3(0.0f, 1.0f, 0.0f));

			glm::quat newRotation = yawQuat * transform.rotation * pitchQuat;

			transform.rotation = glm::normalize(newRotation);

			transform.isDirty = true;
		}
	};

	class MovementDirectionRotationSynchSystem : public SystemBase
	{
		Query2<MovementDirectionComponent, TransformComponent>* m_query;

	public:
		void onInitialize() override
		{
			m_query = m_entityService->queryBuilder()->buildQuery<
				MovementDirectionComponent, TransformComponent>();
		}

		void onUpdate(float deltaTime) override
		{
			auto& query = *m_query;
			for (sizet i = 0, size = query.size(); i < size; ++i)
			{
				auto& movement = query.componentT1(i);
				auto& transform = query.componentT2(i);

				glm::vec3 inputDirection = movement.direction;
				movement.direction = transform.rotation * inputDirection;
			}
		}
	};

	class MovementSystem : public SystemBase
	{
		Query3<MovementDirectionComponent, MovementSpeedComponent, TransformComponent>* m_query;

	public:
		void onInitialize() override
		{
			m_query = m_entityService->queryBuilder()->buildQuery<
				MovementDirectionComponent, MovementSpeedComponent, TransformComponent>();
		}

		void onUpdate(float deltaTime) override
		{
			auto& query = *m_query;
			for (sizet i = 0, size = query.size(); i < size; ++i)
			{
				auto& transform = query.componentT3(i);
				float speed = query.componentT2(i).speed;
				glm::vec3 direction = query.componentT1(i).direction;
				transform.position += direction * speed * deltaTime;
				//SDEBUG_LOG("DIRECTION %f x %f y %f z\n", direction[0], direction[1], direction[2]);
				//SDEBUG_LOG("POSITION %f x %f y %f z\n",
				//           transform.position[0],
				//           transform.position[1],
				//           transform.position[2]);
				transform.isDirty = true;
			}
		}
	};
}
