#include "CoreSystems.hpp"

#include "engine/VulkanResources.hpp"

namespace spite
{
	void DescriptorUpdateSystem::onInitialize()
	{
		m_descriptorQuery = m_entityService->queryBuilder()->buildQuery<
			DescriptorSetLayoutComponent, DescriptorSetsComponent>();
		m_uboQuery = m_entityService->queryBuilder()->buildQuery<UniformBufferComponent>();

		m_uboSharedQuery = m_entityService->queryBuilder()->buildQuery<
			UniformBufferSharedComponent>();

		//requireComponent(typeid(PipelineComponent));
	}

	void DescriptorUpdateSystem::onUpdate(float deltaTime)
	{
		u32 currentFrame = m_entityService->componentManager()->getSingleton<FrameDataComponent>().
		                                    currentFrame;

		auto device = m_entityService->componentManager()->getSingleton<DeviceComponent>().device;

		auto& descriptorQuery = *m_descriptorQuery;
		auto& uboSharedQuery = *m_uboSharedQuery;
		auto& uboQuery = *m_uboQuery;

		for (sizet i = 0, size = descriptorQuery.size(); i < size; ++i)
		{
			auto& descriptorLayoutComponent = descriptorQuery.componentT1(i);

			auto descriptorType = descriptorLayoutComponent.type;
			auto descriptorStage = descriptorLayoutComponent.stages;

			auto descriptor = descriptorQuery.componentT2(i);
			for (sizet j = 0, sizej = uboSharedQuery.size(); j < sizej; ++j)
			{
				auto& ubo = uboSharedQuery[j];

				if (ubo.descriptorType != descriptorType || ubo.shaderStage != descriptorStage)
				{
					continue;
				}

				updateDescriptorSets(device,
				                     descriptor.descriptorSets[currentFrame],
				                     ubo.ubos[currentFrame].buffer.buffer,
				                     ubo.descriptorType,
				                     ubo.bindingIndex,
				                     ubo.elementSize);
				//SDEBUG_LOG("DESCRIPTOR UPDATED\n")
			}

			for (sizet j = 0, sizej = uboQuery.size(); j < sizej; ++j)
			{
				auto& ubo = uboQuery[j];

				if (ubo.descriptorType != descriptorType || ubo.shaderStage != descriptorStage)
				{
					continue;
				}

				updateDescriptorSets(device,
				                     descriptor.descriptorSets[currentFrame],
				                     ubo.ubos[currentFrame].buffer.buffer,
				                     ubo.descriptorType,
				                     ubo.bindingIndex,
				                     ubo.elementSize);
				//SDEBUG_LOG("DESCRIPTOR UPDATED\n")
			}
		}
	}

}
