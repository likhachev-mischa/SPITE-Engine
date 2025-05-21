#include "CoreSystems.hpp"

#include "engine/VulkanLighting.hpp"

namespace spite
{
	void LightUboCreateSystem::onInitialize()
	{
		Entity lightUboEntity = m_entityService->entityManager()->createEntity("LightUbo");

		auto componentManager = m_entityService->componentManager();

		//PointLightComponent pointLightComponent;
		//pointLightComponent.color = glm::vec3(0.0f, 1.0f, 0.0f);
		//pointLightComponent.intensity = 10.0f;
		//pointLightComponent.radius = 0.0f;

		//componentManager->addComponent(pointLightEntity, pointLightComponent);

		//componentManager->addComponent<TransformComponent>(pointLightEntity);
		//componentManager->addComponent<TransformMatrixComponent>(pointLightEntity);

		//componentManager->addComponent<MovementSpeedComponent>(pointLightEntity);
		//componentManager->addComponent<MovementDirectionComponent>(pointLightEntity);

		auto& physicalDeviceComponent = componentManager->getSingleton<
			PhysicalDeviceComponent>();
		auto queueIndices = componentManager->getSingleton<DeviceComponent>().
		                                      queueFamilyIndices;
		auto gpuAllocator = componentManager->getSingleton<GpuAllocatorComponent>().allocator;

		UniformBufferSharedComponent lightUbo;

		lightUbo.descriptorType = vk::DescriptorType::eUniformBuffer;
		lightUbo.shaderStage = vk::ShaderStageFlagBits::eFragment;
		lightUbo.bindingIndex = 0;

		//will pass viewprojection matrix
		sizet elementSize = sizeof(CombinedLightData);
		lightUbo.elementSize = elementSize;
		lightUbo.elementCount = 1;
		sizet minUboAlignment = physicalDeviceComponent.properties.limits.
		                                                minUniformBufferOffsetAlignment;
		sizet dynamicAlignment = (elementSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		lightUbo.elementAlignment = dynamicAlignment;
		for (sizet i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			lightUbo.ubos[i].buffer = BufferWrapper(elementSize,
			                                         vk::BufferUsageFlagBits::eUniformBuffer,
			                                         vk::MemoryPropertyFlagBits::eHostVisible |
			                                         vk::MemoryPropertyFlagBits::eHostCoherent,
			                                         vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
			                                         queueIndices,
			                                         gpuAllocator);

			lightUbo.ubos[i].mappedMemory = lightUbo.ubos[i].buffer.mapMemory();
		}

		m_entityService->componentManager()->addComponent(lightUboEntity, std::move(lightUbo));
	}
}
