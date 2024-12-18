#include "File.hpp"

#include <fstream>
#include <sstream>

#include <glm/vec3.hpp>

#include "Base/Assert.hpp"
#include "Base/Logging.hpp"
#include "Base/Memory.hpp"

namespace spite
{
	eastl::vector<char, spite::HeapAllocator> readBinaryFile(cstring filename,
	                                                         const spite::HeapAllocator& allocator)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		SASSERT(file.is_open())

		sizet fileSize = (sizet)file.tellg();

		eastl::vector<char, spite::HeapAllocator> buffer(allocator);
		buffer.resize(fileSize);

		file.seekg(0);
		file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

		file.close();

		return buffer;
	}

	void readModelInfoFile(cstring filename, eastl::vector<glm::vec3, spite::HeapAllocator>& vertices,
	                       eastl::vector<u32, spite::HeapAllocator>& indices)
	{
		vertices.clear();
		indices.clear();

		std::ifstream file(filename, std::ios::in);
		SASSERT(file.is_open())

		std::string str;

		while (std::getline(file, str))
		{
			char key = str[0];
			str.erase(0, 1);
			std::istringstream iss(str);
			if (key == 'v')
			{
				glm::vec3 vertex;
				for (int i = 0; i < 3; ++i)
				{
					iss >> vertex[i];
				}
				vertices.push_back(vertex);
			}
			else if (key == 'f')
			{
				int index;
				while (iss >> index)
				{
					indices.push_back(index);
				}
			}
		}
	}
}
