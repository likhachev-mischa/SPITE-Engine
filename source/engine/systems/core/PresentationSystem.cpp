#include "CoreSystems.hpp"

#include "engine/VulkanRendering.hpp"

namespace spite
{
	void PresentationSystem::onInitialize()
	{
		//m_pipelineQuery = m_entityService->queryBuilder()->buildQuery<PipelineComponent>();

		//m_modelQuery = m_entityService->queryBuilder()->buildQuery<
		//	MeshComponent, PipelineReference>();

		//SDEBUG_LOG("RENDER SYSTEM INIT\n")
		//requireComponent(typeid(MeshComponent));
		//requireComponent(typeid(PipelineComponent));
	}

	void PresentationSystem::onUpdate(float deltaTime)
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

		//GeometryFramebufferComponent& mainFbComponent = m_entityService->componentManager()->
		//	singleton<GeometryFramebufferComponent>();
		//vk::RenderPass mainRenderPass = m_entityService->componentManager()->singleton<
		//	GeometryRenderPassComponent>().renderPass;

		//DepthFramebufferComponent& depthFbComponent = m_entityService->componentManager()->
		//	singleton<DepthFramebufferComponent>();
		//vk::RenderPass depthRenderPass = m_entityService->componentManager()->singleton<
		//	DepthRenderPassComponent>().renderPass;

		Entity geometryRenderPassEntity = m_entityService->entityManager()->getNamedEntity("GeometryRenderPass");
		auto& geometryFbComponent = m_entityService->componentManager()->getComponent<FramebufferComponent>(geometryRenderPassEntity);
		vk::RenderPass geometryRenderPass = m_entityService->componentManager()->getComponent<RenderPassComponent>(geometryRenderPassEntity).renderPass;

		Entity depthRenderPassEntity = m_entityService->entityManager()->getNamedEntity("DepthRenderPass");
		auto& depthFbComponent = m_entityService->componentManager()->getComponent<FramebufferComponent>(depthRenderPassEntity);
		vk::RenderPass depthRenderPass = m_entityService->componentManager()->getComponent<RenderPassComponent>(depthRenderPassEntity).renderPass;

		Entity lightRenderPassEntity = m_entityService->entityManager()->getNamedEntity("LightRenderPass");
		auto& lightFbComponent = m_entityService->componentManager()->getComponent<FramebufferComponent>(lightRenderPassEntity);
		vk::RenderPass lightRenderPass = m_entityService->componentManager()->getComponent<RenderPassComponent>(lightRenderPassEntity).renderPass;
		auto& gbuffer = m_entityService->componentManager()->getSingleton<GBufferComponent>();

		SynchronizationComponent& synchronizationComponent = m_entityService->componentManager()->
			getSingleton<SynchronizationComponent>();

		CommandBufferComponent& cbComponent = m_entityService->componentManager()->getSingleton<
			CommandBufferComponent>();

		DepthImageComponent& depthImageComponent = m_entityService->componentManager()->getSingleton<DepthImageComponent>();

		//auto& pipelineQuery = *m_pipelineQuery;
		//auto& modelQuery = *m_modelQuery;

		//beginSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame],
		//                            geometryRenderPass,
		//                            geometryFbComponent.framebuffers[imageIndex]);
		//for (sizet i = 0, size = pipelineQuery.size(); i < size; ++i)
		//{
		//	auto& pipeline = pipelineQuery[i];

		//	auto layoutComponent = m_entityService->componentManager()->getComponent<
		//		PipelineLayoutComponent>(pipeline.pipelineLayoutEntity);

		//	//TODO assemble descriptor sets for many shaders
		//	DescriptorSetsComponent& descriptorSets = m_entityService->componentManager()->
		//		getComponent<DescriptorSetsComponent>(
		//			layoutComponent.descriptorSetLayoutEntities[0]);

		//	Entity pipelineEntity = pipelineQuery.owner(i);

		//	for (sizet j = 0, size2 = modelQuery.size(); j < size2; ++j)
		//	{
		//		PipelineReference& pipelineRef = modelQuery.componentT2(j);
		//		//if model is referenced to this pipeline
		//		if (pipelineEntity != pipelineRef.pipelineEntity)
		//		{
		//			continue;
		//		}

		//		u32 offsets = 0;
		//		MeshComponent& meshComponent = modelQuery.componentT1(j);

		//		auto& transformMatrixComponent = m_entityService->componentManager()->getComponent<
		//			TransformMatrixComponent>(modelQuery.owner(j));
		//		cbComponent.geometryBuffers[currentFrame].pushConstants(
		//			layoutComponent.layout,
		//			vk::ShaderStageFlagBits::eVertex,
		//			0,
		//			sizeof(glm::mat4),
		//			&transformMatrixComponent.matrix);

		//		recordSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame],
		//		                             pipeline.pipeline,
		//		                             layoutComponent.layout,
		//		                             {descriptorSets.descriptorSets[currentFrame]},
		//		                             extent,
		//		                             meshComponent.indexBuffer.buffer,
		//		                             meshComponent.vertexBuffer.buffer,
		//		                             &offsets,
		//		                             0,
		//		                             meshComponent.indexCount);
		//	}
		//}
		//endSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame]);

		//recordPrimaryCommandBuffer(cbComponent.primaryBuffers[currentFrame],
		//                           extent,
		//                           geometryRenderPass,
		//                           geometryFbComponent.framebuffers[imageIndex],
		//                           depthRenderPass,
		//                           depthFbComponent.framebuffers[imageIndex],
		//                           {cbComponent.geometryBuffers[currentFrame]},
		//                           {cbComponent.depthBuffers[currentFrame]},
		//                           swapchainComponent.images[imageIndex],depthImageComponent.image.image);

		vk::CommandBuffer primaryCb = cbComponent.primaryBuffers[currentFrame];
		beginCommandBuffer(primaryCb);

		recordDepthPass(primaryCb, cbComponent.depthBuffers[currentFrame], depthRenderPass, depthFbComponent.framebuffers[imageIndex], swapchainComponent.extent, depthImageComponent.image.image);
		recordGeometryPass(primaryCb, cbComponent.geometryBuffers[currentFrame], geometryRenderPass, geometryFbComponent.framebuffers[imageIndex], swapchainComponent.extent, { gbuffer.positionImage.image,gbuffer.normalsImage.image,gbuffer.albedoImage.image });
		recordLightPass(primaryCb, cbComponent.lightBuffers[currentFrame], lightRenderPass, lightFbComponent.framebuffers[imageIndex], swapchainComponent.extent, swapchainComponent.images[imageIndex]);

		endCommandBuffer(primaryCb);

		QueueComponent& queueComponent = m_entityService->componentManager()->getSingleton<
			QueueComponent>();

		drawFrame({ cbComponent.primaryBuffers[currentFrame] },
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
