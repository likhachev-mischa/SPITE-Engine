#pragma once

#include <vector>

#include <EASTL/vector.h>

#include "Base/Platform.hpp"

namespace spite
{
	struct Vertex;
	class HeapAllocator;

	std::vector<char> readBinaryFile(cstring filename);

	void readModelInfoFile(
		cstring filename,
		eastl::vector<Vertex, spite::HeapAllocator>& vertices,
		eastl::vector<u32, spite::HeapAllocator>& indices,
		const spite::HeapAllocator& allocator);

}
