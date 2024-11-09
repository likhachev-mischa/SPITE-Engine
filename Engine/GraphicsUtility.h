#pragma once
#include <stdexcept>
#include <vector>
#include <string>
#include <fstream>
#include <EASTL/vector.h>
#include <glm/glm.hpp>

glm::vec4 lerp(const glm::vec4& a, const glm::vec4& b, float t);

float magnitudeSqr(const glm::vec4& vec);

std::vector<char> readBinaryFile(const std::string& filename);

template <class TAllocator = eastl::allocator>
void readModelInfoFile(const std::string& filename, eastl::vector<glm::vec2, TAllocator>& vertices,
                       eastl::vector<uint16_t, TAllocator>& indices)
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
				vertex[i] = atof(str.substr(0, pos).c_str());
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
