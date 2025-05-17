#include "CoreSystems.hpp"

#include "engine/VulkanRendering.hpp"

namespace spite
{

	void GeometryPassSystem::onInitialize()
	{
		m_pipelineQuery = m_entityService->queryBuilder()->buildQuery<PipelineComponent>();
		m_modelQuery = m_entityService->queryBuilder()->buildQuery<MeshComponent, PipelineReference>();

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

		Entity geometryRenderPassEntity = m_entityService->entityManager()->getNamedEntity("GeometryRenderPass");
		auto& geometryFbComponent = m_entityService->componentManager()->getComponent<FramebufferComponent>(geometryRenderPassEntity);
		vk::RenderPass geometryRenderPass = m_entityService->componentManager()->getComponent<RenderPassComponent>(geometryRenderPassEntity).renderPass;

		CommandBufferComponent& cbComponent = m_entityService->componentManager()->getSingleton<
			CommandBufferComponent>();

		auto& pipelineQuery = *m_pipelineQuery;
		auto& modelQuery = *m_modelQuery;
		beginSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame],
		                            geometryRenderPass,
		                            geometryFbComponent.framebuffers[imageIndex]);
		for (sizet i = 0, size = pipelineQuery.size(); i < size; ++i)
		{
			auto& pipeline = pipelineQuery[i];

			auto layoutComponent = m_entityService->componentManager()->getComponent<
				PipelineLayoutComponent>(pipeline.pipelineLayoutEntity);

			//TODO assemble descriptor sets for many shaders
			DescriptorSetsComponent& descriptorSets = m_entityService->componentManager()->
				getComponent<DescriptorSetsComponent>(
					layoutComponent.descriptorSetLayoutEntities[0]);

			Entity pipelineEntity = pipelineQuery.owner(i);

			for (sizet j = 0, size2 = modelQuery.size(); j < size2; ++j)
			{
				PipelineReference& pipelineRef = modelQuery.componentT2(j);
				//if model is referenced to this pipeline
				if (pipelineEntity != pipelineRef.pipelineEntity)
				{
					continue;
				}

				u32 offsets = 0;
				MeshComponent& meshComponent = modelQuery.componentT1(j);

				auto& transformMatrixComponent = m_entityService->componentManager()->getComponent<
					TransformMatrixComponent>(modelQuery.owner(j));
				cbComponent.geometryBuffers[currentFrame].pushConstants(
					layoutComponent.layout,
					vk::ShaderStageFlagBits::eVertex,
					0,
					sizeof(glm::mat4),
					&transformMatrixComponent.matrix);

				recordSecondaryCommandBuffer(cbComponent.geometryBuffers[currentFrame],
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
		endCommandBuffer(cbComponent.geometryBuffers[currentFrame]);
	}
}
