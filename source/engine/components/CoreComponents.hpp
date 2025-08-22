#pragma once
#include "base/Math.hpp"

#include "base/CollectionAliases.hpp"

#include "ecs/core/Entity.hpp"
#include "ecs/core/IComponent.hpp"

namespace spite
{
	struct TransformComponent : IComponent
	{
		glm::vec3 position = {0.0f, 0.0f, 0.0f};
		glm::quat rotation = glm::quat();
		glm::vec3 scale = {1.0f, 1.0f, 1.0f};
	};

	struct TransformRelationsComponent : IComponent
	{
		Entity parent = Entity::undefined();
		heap_vector<Entity> children;
	};

	struct TransformMatrixComponent : IComponent
	{
		glm::mat4 matrix = glm::mat4(1.0f);
	};

	struct CameraMatricesSingleton : ISingletonComponent
	{
		glm::mat4 view;
		glm::mat4 projection;
	};
}
