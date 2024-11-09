#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <EASTL/vector.h>

#include "Memory.hpp"
#include "Platform.hpp"

struct Model2D
{
	Model2D() = default;

	Model2D(eastl::vector<glm::vec2,spite::HeapAllocator> vertices, eastl::vector<u16,spite::HeapAllocator> indices): vertices(std::move(vertices)),
	                                                                         indices(std::move(indices))
	{
	}

	const eastl::vector<glm::vec2,spite::HeapAllocator> vertices;
	const eastl::vector<u16,spite::HeapAllocator> indices;
};
