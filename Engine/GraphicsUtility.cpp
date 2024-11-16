#include "GraphicsUtility.hpp"
#include <fstream>
#include <EASTL/vector.h>


glm::vec4 lerp(const glm::vec4& a, const glm::vec4& b, float t)
{
	return glm::vec4{
		a[0] + (b[0] - a[0]) * t,
		a[1] + (b[1] - a[1]) * t,
		a[2] + (b[2] - a[2]) * t,
		a[3] + (b[3] - a[3]) * t,
	};
}

float magnitudeSqr(const glm::vec4& vec)
{
	return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] + vec[3] * vec[3];
}

std::vector<char> readBinaryFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

void readModelInfoFile(const std::string& filename, std::vector<glm::vec2>& vertices, std::vector<uint16_t>& indices)
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

