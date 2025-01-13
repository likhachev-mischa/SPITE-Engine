#pragma once
#include <glm/glm.hpp>

namespace spite
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
	};

	struct FragmentData
	{
		glm::vec4 color{0.0f, 1.0f, 0.0f, 1.0f};
	};
}
