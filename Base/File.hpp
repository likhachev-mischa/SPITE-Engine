#pragma once
#include <EASTL/vector.h>

namespace spite
{
	class HeapAllocator;

	eastl::vector<char, spite::HeapAllocator> readBinaryFile(const char* filename,
	                                                         const spite::HeapAllocator& allocator);
}
