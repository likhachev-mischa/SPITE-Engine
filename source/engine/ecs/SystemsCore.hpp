#pragma once
#include <EASTL/vector.h>

#include "Base/Platform.hpp"

#include "Engine/ECS/ComponentsCore.hpp"


namespace spite
{
	struct FragmentData;
	class HeapAllocator;

	void updateTransformMatricesSystem(const eastl::vector<Transform, spite::HeapAllocator>& transforms,
	                                   eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices);

	void updateCameraSystem(const CameraData& cameraData,
	                        const eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices,
	                        const u16 cameraIdx, CameraMatrices& cameraMatrices);

	void updateTransformUboSystem(void* memory, sizet elementAlignment,
	                              const eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices);

	void updateCameraUboSystem(void* memory, const CameraMatrices& matrices);
	void updateFragUboSystem(void* memory, sizet elementAlignment,
		const eastl::vector<FragmentData, spite::HeapAllocator>& fragmentDatas);
}
