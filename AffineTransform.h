#pragma once
#include <glm/glm.hpp>


namespace affine
{
	glm::mat4 identity();

	glm::mat4 translation(float x, float y);

	glm::mat4 rotationX(float angle);

	glm::mat4 rotationY(float angle);

	glm::mat4 rotationZ(float angle);

	glm::mat4 scale(float value);
}
