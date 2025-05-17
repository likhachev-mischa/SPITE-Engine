#pragma once

#include "base/Math.hpp"
#include "ecs/Core.hpp"

namespace spite
{
	struct TransformComponent : IComponent
	{
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
		Entity parent = Entity::undefined(); // Optional parent for hierarchies

		bool isDirty = true;
	};

	struct TransformMatrixComponent : IComponent
	{
		glm::mat4 matrix = glm::mat4(1.0f);
		//glm::mat4 normalMatrix = glm::mat4(1.0f);
		//bool isDirty = true;
	};

	struct CameraSingleton: ISingletonComponent
	{
		Entity camera = Entity::undefined();
	};

	struct CameraDataComponent : IComponent
	{
		float fov = glm::radians(45.0f);
		float nearPlane = 0.1f;
		float farPlane = 10.0f;

		bool isDirty = true;
	};

	struct PointLightComponent : IComponent
	{
		glm::vec3 color;
		float intensity;
		float radius;
	};

	struct CameraMatricesComponent : IComponent
	{
		glm::mat4 view;
		glm::mat4 projection;
	};

	struct MovementSpeedComponent: IComponent
	{
		float speed = 1.0f;
	};

	struct MovementDirectionComponent : IComponent
	{
		glm::vec3 direction{};
	};

	struct ControllableEntitySingleton: ISingletonComponent
	{
		Entity entity;
	};

}
