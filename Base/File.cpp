#include "File.hpp"

#include <fstream>
#include <sstream>

#include <glm/vec3.hpp>

#include "Base/Assert.hpp"
#include "Base/Common.hpp"
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

	void readModelInfoFile(
		cstring filename,
		eastl::vector<Vertex, spite::HeapAllocator>& vertices,
		eastl::vector<u32, spite::HeapAllocator>& indices,
		const spite::HeapAllocator& allocator)
	{
		vertices.clear();
		indices.clear();

		eastl::vector<glm::vec3, spite::HeapAllocator> tempPositions(allocator);
		eastl::vector<glm::vec3, spite::HeapAllocator> tempNormals(allocator);
		eastl::vector<u32, spite::HeapAllocator> positionIndices(allocator);
		eastl::vector<u32, spite::HeapAllocator> normalIndices(allocator);

		std::ifstream file(filename, std::ios::in);
		SASSERT(file.is_open())

		std::string line;

		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '#')
				continue;

			std::istringstream iss(line);
			std::string prefix;
			iss >> prefix;

			if (prefix == "v")
			{
				glm::vec3 vertex;
				iss >> vertex.x >> vertex.y >> vertex.z;
				tempPositions.push_back(vertex);
			}
			else if (prefix == "vn")
			{
				glm::vec3 normal;
				iss >> normal.x >> normal.y >> normal.z;
				tempNormals.push_back(normal);
			}
			else if (prefix == "f")
			{
				std::string vertexInfo;
				while (iss >> vertexInfo)
				{
					std::istringstream viss(vertexInfo);
					std::string vIndexStr, vtIndexStr, vnIndexStr;

					std::getline(viss, vIndexStr, '/');

					if (viss.peek() != '/')
					{
						std::getline(viss, vtIndexStr, '/');
					}
					else
					{
						viss.get();
					}

					std::getline(viss, vnIndexStr, '/');

					int vIndex = std::stoi(vIndexStr) - 1;
					positionIndices.push_back(static_cast<u32>(vIndex));

					if (!vnIndexStr.empty())
					{
						int vnIndex = std::stoi(vnIndexStr) - 1;
						normalIndices.push_back(static_cast<u32>(vnIndex));
					}
					else
					{
						normalIndices.push_back(0);
					}
				}
			}
		}

		for (sizet i = 0; i < positionIndices.size(); ++i)
		{
			Vertex vertex = {};
			vertex.position = tempPositions[positionIndices[i]];
			vertex.normal = tempNormals[normalIndices[i]];
			vertices.push_back(vertex);
			indices.push_back(static_cast<u32>(i));
		}
	}
}
