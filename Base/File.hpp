#pragma once
#include <string>
#include <vector>

#include <EASTL/vector.h>

#include <glm/vec2.hpp>

#include "Base/Platform.hpp"

namespace spite
{
	class HeapAllocator;

	eastl::vector<char, spite::HeapAllocator> readBinaryFile(const char* filename,
	                                                         const spite::HeapAllocator& allocator);

	void readModelInfoFile(const std::string& filename, std::vector<glm::vec2>& vertices, std::vector<u32>& indices);
}
