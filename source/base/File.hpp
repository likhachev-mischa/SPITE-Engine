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
#include "base/memory/ScratchAllocator.hpp"

namespace spite
{

	//nlohmann json templated parser
	template<typename T>
	T parseJson(const cstring filePath)
	{
		std::ifstream file(filePath);
		SASSERTM(file.is_open(), "Error opening %s json", filePath);

		nlohmann::json reader;
		file >> reader;
		T obj;

		try
		{
			obj= reader.get<T>();
		}
		catch (const nlohmann::json::exception& e)
		{
			std::cerr << "json conversion error: " << e.what() << std::endl;
		}

		return obj;
	}

	u8* loadTexture(const cstring path, int& width, int& height, int& channels);

	void freeTexture(u8* pixels);


	void importModelAssimp(const cstring filename,
		scratch_vector<Vertex>& vertices,
		scratch_vector<u32>& indices);

	std::vector<char> readBinaryFile(cstring filename);

	void readModelInfoFile(
		cstring filename,
		eastl::vector<Vertex, spite::HeapAllocator>& vertices,
		eastl::vector<u32, spite::HeapAllocator>& indices,
		const spite::HeapAllocator& allocator);

}
