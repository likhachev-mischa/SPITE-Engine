#include "CoreSystems.hpp"

#include "engine/VulkanRendering.hpp"

namespace spite
{
	void GeometryPassSystem::onInitialize()
	{
		m_modelQuery = m_entityService->queryBuilder()->buildQuery<MeshComponent>();
		//m_pipelineQuery = m_entityService->queryBuilder()->buildQuery<PipelineComponent>();
		//m_modelQuery = m_entityService->queryBuilder()->buildQuery<
		//	MeshComponent, PipelineReference>();

		//requireComponent(typeid(PipelineComponent));
		//requireComponent(typeid(MeshComponent));
	}

	void GeometryPassSystem::onUpdate(float deltaTime)
	{
		FrameDataComponent& frameData = m_entityService->componentManager()->getSingleton<
			FrameDataComponent>();

		u32 imageIndex = frameData.imageIndex;
		u32 currentFrame = frameData.currentFrame;

		SwapchainComponent& swapchainComponent = m_entityService->componentManager()->getSingleton<
			SwapchainComponent>();
		vk::Extent2D extent = swapchainComponent.extent;

		Entity geometryRenderPassEntity = m_entityService->entityManager()->getNamedEntity(
			"GeometryRenderPass");
		auto& geometryFbComponent = m_entityService->componentManager()->getComponent<
			FramebufferComponent>(geometryRenderPassEntity);
		vk::RenderPass geometryRenderPass = m_entityService->componentManager()->getComponent<
			RenderPassComponent>(geometryRenderPassEntity).renderPass;
		Entity geomPipelineEntity = m_entityService->entityManager()->getNamedEntity(
			"GeometryPipeline");
		const auto& pipelineComponent = m_entityService->componentManager()->getComponent<
			PipelineComponent>(geomPipelineEntity);
		const auto& pipelineLayoutComponent = m_entityService->componentManager()->getComponent<
			PipelineLayoutComponent>(pipelineComponent.pipelineLayoutEntity);


		CommandBufferComponent& cbComponent = m_entityService->componentManager()->getSingleton<
			CommandBufferComponent>();
		vk::Device device = m_entityService->componentManager()->getSingleton<DeviceComponent>().
		                                     device;

		auto& modelQuery = *m_modelQuery;

		std::vector<vk::DescriptorSet> descriptorSets;
		for (const auto& descriptorSetLayoutEntity : pipelineLayoutComponent.
		     descriptorSetLayoutEntities)
		{
			if (m_entityService->componentManager()->hasComponent<DescriptorSetsComponent>(
				descriptorSetLayoutEntity))
				descriptorSets.push_back(
					m_entityService->componentManager()->getComponent<DescriptorSetsComponent>(
						descriptorSetLayoutEntity).descriptorSets[currentFrame]);
		}
		//TEXTURE
		sizet textureDescriptorIdx = descriptorSets.size();
		descriptorSets.emplace_back();

		beginSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame],
		                            geometryRenderPass,
		                            geometryFbComponent.framebuffers[imageIndex]);

		u32 offsets = 0;
		for (sizet i = 0, size = modelQuery.size(); i < size; ++i)
		{
			auto& transformMatrixComponent = m_entityService->componentManager()->getComponent<
				TransformMatrixComponent>(modelQuery.owner(i));
			cbComponent.geometryBuffers[currentFrame].pushConstants(
				pipelineLayoutComponent.layout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(glm::mat4),
				&transformMatrixComponent.matrix);

			const auto& textureComponent = m_entityService->componentManager()->getComponent<TextureComponent>(modelQuery.owner(i));
			const auto& textureDescriptorComponent = m_entityService->componentManager()->getComponent<DescriptorSetsComponent>(textureComponent.descriptorEntity);

			const auto& textureDescriptor = textureDescriptorComponent.descriptorSets[currentFrame];
			descriptorSets[textureDescriptorIdx] = textureDescriptor;

			const auto& meshComponent = modelQuery[i];

			recordSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame],
			                             pipelineComponent.pipeline,
			                             pipelineLayoutComponent.layout,
			                             descriptorSets,
			                             extent,
			                             meshComponent.indexBuffer.buffer,
			                             meshComponent.vertexBuffer.buffer,
			                             &offsets,
			                             0,
			                             meshComponent.indexCount);
		}

		endCommandBuffer(cbComponent.geometryBuffers[currentFrame]);


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

		//	//DescriptorSetsComponent& descriptorSets = m_entityService->componentManager()->
		//	//	getComponent<DescriptorSetsComponent>(
		//	//		layoutComponent.descriptorSetLayoutEntities[0]);
		//	vk::DescriptorSet textureDescriptorSet;
		//	std::vector<vk::DescriptorSet> descriptorSets;
		//	for (const auto& descriptorSetLayoutEntity : layoutComponent.
		//	     descriptorSetLayoutEntities)
		//	{
		//		//HARDCODED FRAGMENT TEXTURE WRITE;
		//		vk::DescriptorSet set = m_entityService->componentManager()->getComponent<
		//			DescriptorSetsComponent>(descriptorSetLayoutEntity).descriptorSets[
		//			currentFrame];
		//		if (m_entityService->componentManager()->getComponent<
		//				DescriptorSetLayoutComponent>(descriptorSetLayoutEntity).stages ==
		//			vk::ShaderStageFlagBits::eFragment)
		//		{
		//			textureDescriptorSet = set;
		//		}
		//		descriptorSets.push_back(set);
		//	}

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

		//beginSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame],
		//                            geometryRenderPass,
		//                            geometryFbComponent.framebuffers[imageIndex]);
		//		auto& transformMatrixComponent = m_entityService->componentManager()->getComponent<
		//			TransformMatrixComponent>(modelQuery.owner(j));
		//		cbComponent.geometryBuffers[currentFrame].pushConstants(
		//			layoutComponent.layout,
		//			vk::ShaderStageFlagBits::eVertex,
		//			0,
		//			sizeof(glm::mat4),
		//			&transformMatrixComponent.matrix);

		//		//descriptorWrite
		//		if (m_entityService->componentManager()->hasComponent<TextureComponent>(modelQuery.owner(j)))
		//		{
		//			const auto& textureComponent = m_entityService->componentManager()->getComponent<TextureComponent>(modelQuery.owner(j));
		//			vk::DescriptorImageInfo imageInfo(textureComponent.sampler, textureComponent.imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
		//			vk::WriteDescriptorSet descriptorWrite(textureDescriptorSet,0,0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfo);

		//			device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
		//		}

		//		recordSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame],
		//		                             pipeline.pipeline,
		//		                             layoutComponent.layout,
		//		                             descriptorSets,
		//		                             extent,
		//		                             meshComponent.indexBuffer.buffer,
		//		                             meshComponent.vertexBuffer.buffer,
		//		                             &offsets,
		//		                             0,
		//		                             meshComponent.indexCount);
		//endCommandBuffer(cbComponent.geometryBuffers[currentFrame]);
		//	}
		//}
		//endCommandBuffer(cbComponent.geometryBuffers[currentFrame]);
	}
}
