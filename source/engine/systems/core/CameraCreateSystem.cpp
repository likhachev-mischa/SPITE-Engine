#include "CoreSystems.hpp"

namespace spite
{
	void CameraCreateSystem::onInitialize()
	{
		Entity camera = m_entityService->entityManager()->createEntity();

		auto componentManager = m_entityService->componentManager();

		componentManager->addComponent<TransformComponent>(camera);
		componentManager->addComponent<TransformMatrixComponent>(camera);
		componentManager->addComponent<CameraDataComponent>(camera);
		componentManager->addComponent<CameraMatricesComponent>(camera);

		CameraSingleton cameraSingleton;
		cameraSingleton.camera = camera;
		componentManager->createSingleton(cameraSingleton);

		ControllableEntitySingleton controllableEntitySingleton;
		controllableEntitySingleton.entity = camera;
		componentManager->createSingleton(controllableEntitySingleton);

		auto& physicalDeviceComponent = componentManager->getSingleton<
			PhysicalDeviceComponent>();
		auto queueIndices = componentManager->getSingleton<DeviceComponent>().
		                                      queueFamilyIndices;
		auto gpuAllocator = componentManager->getSingleton<GpuAllocatorComponent>().allocator;

		UniformBufferSharedComponent cameraUbo;

		cameraUbo.descriptorType = vk::DescriptorType::eUniformBuffer;
		cameraUbo.shaderStage = vk::ShaderStageFlagBits::eVertex;
		cameraUbo.bindingIndex = 0;

		//will pass viewprojection matrix
		sizet elementSize = sizeof(glm::mat4);
		cameraUbo.elementSize = elementSize;
		cameraUbo.elementCount = 1;
		sizet minUboAlignment = physicalDeviceComponent.properties.limits.
		                                                minUniformBufferOffsetAlignment;
		sizet dynamicAlignment = (elementSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		cameraUbo.elementAlignment = dynamicAlignment;
		for (sizet i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			cameraUbo.ubos[i].buffer = BufferWrapper(elementSize,
			                                         vk::BufferUsageFlagBits::eUniformBuffer,
			                                         vk::MemoryPropertyFlagBits::eHostVisible |
			                                         vk::MemoryPropertyFlagBits::eHostCoherent,
			                                         vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
			                                         queueIndices,
			                                         gpuAllocator);

			cameraUbo.ubos[i].mappedMemory = cameraUbo.ubos[i].buffer.mapMemory();
		}

		m_entityService->componentManager()->addComponent(camera, std::move(cameraUbo));
	}
}
