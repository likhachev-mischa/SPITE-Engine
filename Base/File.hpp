#pragma once
#include <string>

#include <EASTL/vector.h>

#include <glm/vec3.hpp>

#include "Base/Platform.hpp"

namespace spite
{
	class HeapAllocator;

	eastl::vector<char, spite::HeapAllocator> readBinaryFile(cstring filename,
	                                                         const spite::HeapAllocator& allocator);

	void readModelInfoFile(cstring filename, eastl::vector<glm::vec3,spite::HeapAllocator>& vertices, eastl::vector<u32,spite::HeapAllocator>& indices);
}
