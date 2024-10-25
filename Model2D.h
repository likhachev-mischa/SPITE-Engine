#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Model2D
{
	Model2D() = default;

	Model2D(std::vector<glm::vec2> vertices, std::vector<uint16_t> indices): vertices(std::move(vertices)),
	                                                                         indices(std::move(indices))
	{
	}

	const std::vector<glm::vec2> vertices;
	const std::vector<uint16_t> indices;
};
