#include "SystemsCore.hpp"

#include <glm/gtc/quaternion.hpp>

#include "Base/Memory.hpp"

namespace spite
{
	void updateTransformMatricesSystem(const eastl::vector<Transform, spite::HeapAllocator>& transforms,
	                                   eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices)
	{
		for (sizet i = 0, size = transforms.size(); i < size; ++i)
		{
			glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), transforms[i].position);
			glm::mat4 rotationMatrix = glm::mat4_cast(transforms[i].rotation);
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), transforms[i].scale);

			transformMatrices[i].matrix = translationMatrix * rotationMatrix * scaleMatrix;
		}
	}

	void updateCameraSystem(const CameraData& cameraData,
	                        const eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices,
	                        const u16 cameraIdx,
	                        CameraMatrices& cameraMatrices)
	{
		cameraMatrices.projection = glm::perspective(glm::radians(cameraData.fov), cameraData.aspectRatio,
		                                             cameraData.nearPlane, cameraData.farPlane);

		cameraMatrices.view = glm::inverse(transformMatrices[cameraIdx].matrix);
	}

	void updateTransformUboSystem(void* memory, sizet elementAlignment,
	                              const eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices)
	{
		auto dynamicData = static_cast<u8*>(memory);
		for (sizet i = 0, size = transformMatrices.size(); i < size; ++i)
		{
			auto transform = reinterpret_cast<TransformMatrix*>(dynamicData + i * elementAlignment);
			*transform = transformMatrices[i];
		}
	}

	void updateCameraUboSystem(void* memory, const CameraMatrices& matrices)
	{
		memcpy(memory, &matrices, sizeof(CameraMatrices));
	}
}
