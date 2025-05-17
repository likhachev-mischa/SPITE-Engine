#include "CoreSystems.hpp"
#include "engine/VulkanLighting.hpp"

namespace spite
{
	void LightPassSystem::onInitialize()
	{
		//requireComponent(typeid(PipelineComponent));
		//requireComponent(typeid(MeshComponent));
	}

	void LightPassSystem::onUpdate(float deltaTime)
	{
		FrameDataComponent& frameData = m_entityService->componentManager()->getSingleton<
			FrameDataComponent>();

		u32 imageIndex = frameData.imageIndex;
		u32 currentFrame = frameData.currentFrame;

		SwapchainComponent& swapchainComponent = m_entityService->componentManager()->getSingleton<
			SwapchainComponent>();

		CommandBufferComponent& cbComponent = m_entityService->componentManager()->getSingleton<
			CommandBufferComponent>();

		Entity lightRenderPassEntity = m_entityService->entityManager()->getNamedEntity("LightRenderPass");

		auto& lightFbComponent = m_entityService->componentManager()->getComponent<FramebufferComponent>(lightRenderPassEntity);
		vk::RenderPass lightRenderPass = m_entityService->componentManager()->getComponent<RenderPassComponent>(lightRenderPassEntity).renderPass;

		//DepthFramebufferComponent& depthFbComponent = m_entityService->componentManager()->
		//	singleton<DepthFramebufferComponent>();
		//vk::RenderPass depthRenderPass = m_entityService->componentManager()->singleton<
		//	DepthRenderPassComponent>().renderPass;

		Entity lightPipelineEntity = m_entityService->entityManager()->getNamedEntity(
			"LightPipeline");
		auto& lightPipelineComponent = m_entityService->componentManager()->getComponent<
			PipelineComponent>(lightPipelineEntity);
		vk::Pipeline lightPipeline = lightPipelineComponent.pipeline;
		auto& lightPipelineLayoutComponent = m_entityService->componentManager()->getComponent<
			PipelineLayoutComponent>(lightPipelineComponent.pipelineLayoutEntity);
		vk::PipelineLayout lightPipelineLayout = lightPipelineLayoutComponent.layout;
		auto& descriptorSets = m_entityService->componentManager()->getComponent<
			                                        DescriptorSetsComponent>(
			                                        lightPipelineLayoutComponent.
			                                        descriptorSetLayoutEntities[0]).
		                                        descriptorSets;

		u32 offsets = 0;
		recordLightCommandBuffer(cbComponent.lightBuffers[currentFrame],lightRenderPass,lightFbComponent.framebuffers[imageIndex], lightPipeline, lightPipelineLayout, {descriptorSets[currentFrame]}, swapchainComponent.extent, &offsets);

	}
}
