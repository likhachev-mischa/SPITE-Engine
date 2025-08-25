#pragma once

#include <fstream>
#include <vector>

#include "assimp/importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <EASTL/vector.h>

#include <nlohmann/json.hpp>

#include "base/Common.hpp"
#include "Base/Assert.hpp"
#include "Base/Platform.hpp"
#include "CollectionAliases.hpp"

namespace spite
{
	//nlohmann json templated parser
	template <typename T>
	T parseJson(cstring filePath)
	{
		std::ifstream file(filePath);
		SASSERTM(file.is_open(), "Error opening %s json", filePath);

		nlohmann::json reader;
		file >> reader;
		T obj;

		try
		{
			obj = reader.get<T>();
		}
		catch (const nlohmann::json::exception& e)
		{
			std::cerr << "json conversion error: " << e.what() << std::endl;
		}

		return obj;
	}

	u8* loadTexture(cstring path, int& width, int& height, int& channels);

	void freeTexture(u8* pixels);


	void importModelAssimp(cstring filename,
	                       scratch_vector<Vertex>& vertices,
	                       scratch_vector<u32>& indices);

	std::vector<char> readBinaryFile(cstring filename);
	void writeBinaryFile(cstring filename, void* data, sizet size);

	void readModelInfoFile(
		cstring filename,
		eastl::vector<Vertex, spite::HeapAllocator>& vertices,
		eastl::vector<u32, spite::HeapAllocator>& indices,
		const spite::HeapAllocator& allocator);
}
