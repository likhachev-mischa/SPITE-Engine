#include "File.hpp"
#include <vector>
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

		std::vector<char> stagingBuffer(fileSize);

		file.seekg(0);
		file.read(stagingBuffer.data(), static_cast<std::streamsize>(fileSize));

		file.close();

		eastl::vector<char, spite::HeapAllocator> buffer(allocator);
		buffer.resize(fileSize);

		for (sizet i = 0; i < fileSize; ++i)
		{
			buffer[i] = stagingBuffer[i];
		}
		return buffer;
	}

	void readModelInfoFile(const std::string& filename, std::vector<glm::vec2>& vertices, std::vector<u32>& indices)
	{
		vertices.clear();
		indices.clear();

		std::ifstream file(filename, std::ios::in);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file!");
		}
		std::string str;

		while (std::getline(file, str))
		{
			char key = str[0];
			str.erase(0, 1);
			if (key == 'v')
			{
				glm::vec2 vertex;
				for (int i = 0; i < 2; ++i)
				{
					while (str[0] == ' ')
					{
						str.erase(0, 1);
					}
					size_t pos = str.find(' ');
					vertex[i] = static_cast<float>(atof(str.substr(0, pos).c_str()));
					str.erase(0, pos);
				}
				vertices.push_back(vertex);
			}
			else if (key == 'f')
			{
				while (!str.empty())
				{
					while (str[0] == ' ')
					{
						str.erase(0, 1);
					}
					size_t pos = str.find(' ');
					indices.push_back(atoi(str.substr(0, pos).c_str()));
					str.erase(0, pos);
				}
			}
		}
	}
}
