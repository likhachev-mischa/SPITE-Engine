#pragma once

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

#include "Base/Platform.hpp"

namespace spite
{
	struct IComponent
	{
	};

	struct Transform : IComponent
	{
		glm::vec3 position{};
		glm::vec3 scale{1.0f, 1.0f, 1.0f};

		glm::quat rotation{1.0f,0.0f,0.0f,0.0f};
	};

	struct TransformMatrix : IComponent
	{
		glm::mat4 matrix;

		TransformMatrix() : matrix(1.0f)
		{
		}
	};

	struct CameraData //: IComponent
	{
		f32 fov;
		f32 aspectRatio;
		f32 nearPlane;
		f32 farPlane;
	};

	struct CameraMatrices : IComponent
	{
		glm::mat4 view;
		glm::mat4 projection;
	};
}
