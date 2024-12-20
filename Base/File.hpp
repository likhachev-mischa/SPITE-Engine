#pragma once

#include <EASTL/vector.h>

#include "Base/Platform.hpp"

namespace spite
{
	struct Vertex;
	class HeapAllocator;

	eastl::vector<char, spite::HeapAllocator> readBinaryFile(cstring filename,
	                                                         const spite::HeapAllocator& allocator);

	void readModelInfoFile(
		cstring filename,
		eastl::vector<Vertex, spite::HeapAllocator>& vertices,
		eastl::vector<u32, spite::HeapAllocator>& indices,
		const spite::HeapAllocator& allocator);

}
