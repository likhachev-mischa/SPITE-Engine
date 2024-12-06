#include "File.hpp"

#include <fstream>

#include "Base/Assert.hpp"
#include "Base/Logging.hpp"
#include "Base/Memory.hpp"

namespace spite
{
	eastl::vector<char, spite::HeapAllocator> readBinaryFile(const char* filename,
	                                                         const spite::HeapAllocator& allocator)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		SASSERT(file.is_open())

		size_t fileSize = (size_t)file.tellg();
		eastl::vector<char, spite::HeapAllocator> buffer(allocator);
		buffer.reserve(fileSize);

		file.seekg(0);
		buffer.insert(buffer.begin(),
		              std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

		file.close();
		return buffer;
	}
}
