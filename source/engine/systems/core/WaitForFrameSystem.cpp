#include "CoreSystems.hpp"

#include "engine/VulkanRendering.hpp"

namespace spite
{
	void WaitForFrameSystem::onInitialize()
	{
		requireComponent(typeid(PipelineComponent));
	}

	void WaitForFrameSystem::onUpdate(float deltaTime)
	{
		FrameDataComponent& frameData = m_entityService->componentManager()->singleton<
			FrameDataComponent>();

		u32 imageIndex = frameData.imageIndex;
		u32 currentFrame = frameData.currentFrame;

		vk::SwapchainKHR swapchain = m_entityService->componentManager()->singleton<
			SwapchainComponent>().swapchain;

		SynchronizationComponent& synchronizationComponent = m_entityService->componentManager()
			->singleton<SynchronizationComponent>();
		vk::Device device = m_entityService->componentManager()->singleton<DeviceComponent>()
		                                   .device;

		vk::Result result = waitForFrame(device,
		                                 swapchain,
		                                 synchronizationComponent.inFlightFences[currentFrame],
		                                 synchronizationComponent.imageAvailableSemaphores[
			                                 currentFrame],
		                                 imageIndex);
		SASSERT_VULKAN(result);

		frameData.imageIndex = imageIndex;
	}

	
}
