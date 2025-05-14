#include "CoreSystems.hpp"

namespace spite
{
	void CameraUboUpdateSystem::onUpdate(float deltaTime)
	{
		Entity camera = m_entityService->componentManager()->singleton<CameraSingleton>().camera;

		auto& cameraUbo = m_entityService->componentManager()->getComponent<
			UniformBufferSharedComponent>(camera);

		auto& cameraMatrices = m_entityService->componentManager()->getComponent<
			CameraMatricesComponent>(camera);

		u32 currentFrame = m_entityService->componentManager()->singleton<FrameDataComponent>().
		                                    currentFrame;

		glm::mat4 viewProjection = cameraMatrices.projection * cameraMatrices.view;

		memcpy(cameraUbo.ubos[currentFrame].mappedMemory, &viewProjection, cameraUbo.elementSize);
	}

	
}
