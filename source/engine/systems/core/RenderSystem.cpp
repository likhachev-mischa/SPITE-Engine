#include "CoreSystems.hpp"

#include "engine/VulkanRendering.hpp"

namespace spite
{
	void RenderSystem::onInitialize()
	{
		m_pipelineQuery = m_entityService->queryBuilder()->buildQuery<PipelineComponent>();

		m_modelQuery = m_entityService->queryBuilder()->buildQuery<
			MeshComponent, PipelineReference>();

		SDEBUG_LOG("RENDER SYSTEM INIT\n")
		requireComponent(typeid(PipelineComponent));
	}

	void RenderSystem::onUpdate(float deltaTime)
	{
		FrameDataComponent& frameData = m_entityService->componentManager()->getSingleton<
			FrameDataComponent>();

		u32 imageIndex = frameData.imageIndex;
		u32 currentFrame = frameData.currentFrame;

		//SDEBUG_LOG("RENDER SYSTEM UPDATE START\n")

		SwapchainComponent& swapchainComponent = m_entityService->componentManager()->getSingleton<
			SwapchainComponent>();
		vk::SwapchainKHR swapchain = swapchainComponent.swapchain;
		vk::Extent2D extent = swapchainComponent.extent;

		FramebufferComponent& fbComponent = m_entityService->componentManager()->getSingleton<
			FramebufferComponent>();
		vk::RenderPass renderPass = m_entityService->componentManager()->getSingleton<
			RenderPassComponent>().renderPass;

		SynchronizationComponent& synchronizationComponent = m_entityService->componentManager()->
			getSingleton<SynchronizationComponent>();

		CommandBufferComponent& cbComponent = m_entityService->componentManager()->getSingleton<
			CommandBufferComponent>();

		auto& pipelineQuery = *m_pipelineQuery;
		auto& modelQuery = *m_modelQuery;

		beginSecondaryCommandBuffer(cbComponent.secondaryBuffers[currentFrame],
		                            renderPass,
		                            fbComponent.framebuffers[imageIndex]);
		for (sizet i = 0, size = pipelineQuery.size(); i < size; ++i)
		{
			auto& pipeline = pipelineQuery[i];

			auto layoutComponent = m_entityService->componentManager()->getComponent<
				PipelineLayoutComponent>(pipeline.pipelineLayoutEntity);

			//TODO assemble descriptor sets for many shaders
			DescriptorSetsComponent& descriptorSets = m_entityService->componentManager()->
				getComponent<DescriptorSetsComponent>(
					layoutComponent.descriptorSetLayoutEntities[0]);

			Entity pipelineEntity = pipelineQuery.componentOwner(i);

			for (sizet j = 0, size2 = modelQuery.size(); j < size2; ++j)
			{
				PipelineReference& pipelineRef = modelQuery.getComponentT2(j);
				//if model is referenced to this pipeline
				if (pipelineEntity != pipelineRef.pipelineEntity)
				{
					continue;
				}

				u32 offsets = 0;
				MeshComponent& meshComponent = modelQuery.getComponentT1(j);

				auto& transformMatrixComponent = m_entityService->componentManager()->getComponent<
					TransformMatrixComponent>(modelQuery.owner(j));
				cbComponent.secondaryBuffers[currentFrame].pushConstants(
					layoutComponent.layout,
					vk::ShaderStageFlagBits::eVertex,
					0,
					sizeof(glm::mat4),
					&transformMatrixComponent.matrix);

				recordSecondaryCommandBuffer(cbComponent.secondaryBuffers[currentFrame],
				                             pipeline.pipeline,
				                             layoutComponent.layout,
				                             {descriptorSets.descriptorSets[currentFrame]},
				                             extent,
				                             meshComponent.indexBuffer.buffer,
				                             meshComponent.vertexBuffer.buffer,
				                             &offsets,
				                             0,
				                             meshComponent.indexCount);
			}
		}
		endSecondaryCommandBuffer(cbComponent.secondaryBuffers[currentFrame]);

		recordPrimaryCommandBuffer(cbComponent.primaryBuffers[currentFrame],
		                           extent,
		                           renderPass,
		                           fbComponent.framebuffers[imageIndex],
		                           {cbComponent.secondaryBuffers[currentFrame]},
		                           swapchainComponent.images[imageIndex]);

		QueueComponent& queueComponent = m_entityService->componentManager()->getSingleton<
			QueueComponent>();

		drawFrame(cbComponent.primaryBuffers[currentFrame],
		          synchronizationComponent.inFlightFences[currentFrame],
		          synchronizationComponent.imageAvailableSemaphores[currentFrame],
		          synchronizationComponent.renderFinishedSemaphores[currentFrame],
		          queueComponent.graphicsQueue,
		          queueComponent.presentQueue,
		          swapchain,
		          imageIndex);

		//SDEBUG_LOG("FRAME RENDERED\n");
		currentFrame = (currentFrame + 1) % (MAX_FRAMES_IN_FLIGHT);

		frameData.imageIndex = imageIndex;
		frameData.currentFrame = currentFrame;
	}

}
