#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <EASTL/vector.h>

#include "Memory.hpp"
#include "Platform.hpp"

struct Model2D
{
	Model2D() = default;

	Model2D(std::vector<glm::vec2> vertices, std::vector<u16> indices): vertices(std::move(vertices)),
	                                                                         indices(std::move(indices))
	{
	}

	const std::vector<glm::vec2> vertices;
	const std::vector<u16> indices;
};
