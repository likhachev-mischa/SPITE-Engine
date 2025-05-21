#include "File.hpp"

#include <fstream>
#include <sstream>
#include <stb_image.h>
#include <vector>

#include <glm/vec3.hpp>

#include "Base/Assert.hpp"
#include "Base/Common.hpp"
#include "Base/Logging.hpp"
#include "Base/Memory.hpp"

#include "assimp/importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

namespace spite
{
	std::vector<char> readBinaryFile(const cstring filename)

	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		SASSERT(file.is_open())

		sizet fileSize = (sizet)file.tellg();

		std::vector<char> buffer;
		buffer.resize(fileSize);

		file.seekg(0);
		file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

		file.close();

		return buffer;
	}

	u8* loadTexture(const cstring path, int& width, int& height, int& channels)
	{
		u8* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);

		if (pixels == nullptr)
		{
			SASSERTM(pixels != nullptr, "Failed to load texture! %s\n", stbi_failure_reason());
			return nullptr;
		}
		return pixels;
	}

	void freeTexture(u8* pixels)
	{
		stbi_image_free(pixels);
	}

	void importModelAssimp(const cstring filename,
	                       eastl::vector<Vertex, spite::HeapAllocator>& vertices,
	                       eastl::vector<u32, spite::HeapAllocator>& indices)
	{
		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(filename,
			aiProcess_Triangulate | aiProcess_MakeLeftHanded |
			aiProcess_GenNormals | aiProcess_GenUVCoords);

		if (!scene)
		{
			auto error = importer.GetErrorString();

			SASSERTM(scene != nullptr, "ASSIMP ERROR %s", error)
		}

		SASSERTM(scene->HasMeshes(), "file %s has no meshes", filename)

			vertices.clear();
		indices.clear();

		u32 vertexOffset = 0; // Used to offset indices for multiple meshes

		for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[i];

			// Process vertices
			for (unsigned int j = 0; j < mesh->mNumVertices; ++j)
			{
				Vertex vertex{};
				vertex.position = {
					mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z
				};

				SASSERTM(mesh->HasNormals(), "Mesh %s has no normals!");
				vertex.normal = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };

				if (mesh->HasTextureCoords(0)) // Check for the first UV channel
				{
					vertex.uv = {
						mesh->mTextureCoords[0][j].x,
						mesh->mTextureCoords[0][j].y
					};
				}
				// If no UVs, vertex.uv remains (0,0) due to Vertex vertex{}; initialization.

				vertices.push_back(vertex);
			}

			// Process indices
			for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
			{
				aiFace face = mesh->mFaces[j];
				// Assuming aiProcess_Triangulate ensures 3 indices per face
				SASSERTM(face.mNumIndices == 3,
					"Face is not a triangle after aiProcess_Triangulate");
				for (unsigned int k = 0; k < face.mNumIndices; ++k)
				{
					indices.push_back(vertexOffset + face.mIndices[k]);
				}
			}
			vertexOffset += mesh->mNumVertices;
		}
	}

	void readModelInfoFile(const cstring filename,
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
			if (line.empty() || line[0] == '#') continue;

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
