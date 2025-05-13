#include "CoreSystems.hpp"
#include "engine/VulkanRendering.hpp"

namespace spite
{
	void DepthPassSystem::onInitialize()
	{
		m_modelQuery = m_entityService->queryBuilder()->buildQuery<MeshComponent>();

		requireComponent(typeid(DepthPipelineTag));
	}

	void DepthPassSystem::onUpdate(float deltaTime)
	{
		FrameDataComponent& frameData = m_entityService->componentManager()->getSingleton<
			FrameDataComponent>();

		u32 imageIndex = frameData.imageIndex;
		u32 currentFrame = frameData.currentFrame;

		SwapchainComponent& swapchainComponent = m_entityService->componentManager()->getSingleton<
			SwapchainComponent>();
		vk::SwapchainKHR swapchain = swapchainComponent.swapchain;
		vk::Extent2D extent = swapchainComponent.extent;

		CommandBufferComponent& cbComponent = m_entityService->componentManager()->getSingleton<
			CommandBufferComponent>();

		SynchronizationComponent& synchronizationComponent = m_entityService->componentManager()->
			getSingleton<SynchronizationComponent>();

		DepthFramebufferComponent& depthFbComponent = m_entityService->componentManager()->
			getSingleton<DepthFramebufferComponent>();
		vk::RenderPass depthRenderPass = m_entityService->componentManager()->getSingleton<
			DepthRenderPassComponent>().renderPass;

		Entity depthPipelineEntity = m_entityService->entityManager()->getNamedEntity(
			"DepthPipeline");
		auto& depthPipelineComponent = m_entityService->componentManager()->getComponent<
			PipelineComponent>(depthPipelineEntity);
		vk::Pipeline depthPipeline = depthPipelineComponent.pipeline;
		auto& depthPipelineLayoutComponent = m_entityService->componentManager()->getComponent<
			PipelineLayoutComponent>(depthPipelineComponent.pipelineLayoutEntity);
		vk::PipelineLayout depthPipelineLayout = depthPipelineLayoutComponent.layout;
		auto& descriptorSets = m_entityService->componentManager()->getComponent<
			                                        DescriptorSetsComponent>(
			                                        depthPipelineLayoutComponent.
			                                        descriptorSetLayoutEntities[0]).
		                                        descriptorSets;

		beginSecondaryCommandBuffer(cbComponent.depthBuffers[currentFrame],
		                            depthRenderPass,
		                            depthFbComponent.framebuffers[imageIndex]);
		auto& modelQuery = *m_modelQuery;
		for (sizet i = 0, size = modelQuery.size(); i < size; ++i)
		{
			auto& transformMatrixComponent = m_entityService->componentManager()->getComponent<
				TransformMatrixComponent>(modelQuery.owner(i));

			cbComponent.depthBuffers[currentFrame].pushConstants(
				depthPipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(glm::mat4),
				&transformMatrixComponent.matrix);

			u32 offsets = 0;
			MeshComponent& meshComponent = modelQuery[i];
			recordSecondaryCommandBuffer(cbComponent.depthBuffers[currentFrame],
			                             depthPipeline,
			                             depthPipelineLayout,
			                             {descriptorSets[currentFrame]},
			                             extent,
			                             meshComponent.indexBuffer.buffer,
			                             meshComponent.vertexBuffer.buffer,
			                             &offsets,
			                             0,
			                             meshComponent.indexCount);
		}
		endSecondaryCommandBuffer(cbComponent.depthBuffers[currentFrame]);
	}
}
