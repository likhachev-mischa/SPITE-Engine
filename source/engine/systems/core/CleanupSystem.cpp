#include "CoreSystems.hpp"

#include "engine/VulkanDebug.hpp"

namespace spite
{
	void CleanupSystem::onInitialize()
	{
		requireComponent(typeid(CleanupRequest));
	}

	void CleanupSystem::onUpdate(float deltaTime)
	{
		auto componentManager = m_entityService->componentManager();

		AllocationCallbacksComponent& allocationCallbacksComponent = componentManager->getSingleton<
			AllocationCallbacksComponent>();
		vk::AllocationCallbacks* allocationCallbacks = &allocationCallbacksComponent.
			allocationCallbacks;

		GpuAllocatorComponent& gpuAllocatorComponent = componentManager->getSingleton<
			GpuAllocatorComponent>();

		vk::Instance instance = componentManager->getSingleton<VulkanInstanceComponent>().instance;
		//vk::PhysicalDevice physicalDevice = componentManager->getSingleton<PhysicalDeviceComponent>().device;
		vk::Device device = componentManager->getSingleton<DeviceComponent>().device;
		vk::SurfaceKHR surface = componentManager->getSingleton<SurfaceComponent>().surface;

		SwapchainComponent swapchainComponent = componentManager->getSingleton<SwapchainComponent>();
		vk::SwapchainKHR swapchain = swapchainComponent.swapchain;
		//vk::RenderPass renderPass = componentManager->singleton<GeometryRenderPassComponent>().
		//                                              renderPass;
		//GeometryFramebufferComponent& framebufferComponent = componentManager->singleton<
		//	GeometryFramebufferComponent>();
		CommandPoolComponent& commandPoolComponent = componentManager->getSingleton<
			CommandPoolComponent>();
		SynchronizationComponent& synchronizationComponent = componentManager->getSingleton<
			SynchronizationComponent>();

		auto queryBuilder = m_entityService->queryBuilder();
		Query1<ShaderComponent>& shaderQuery = *queryBuilder->buildQuery<ShaderComponent>();
		Query1<DescriptorSetLayoutComponent>& descriptorLayoutQuery = *queryBuilder->buildQuery<
			DescriptorSetLayoutComponent>();
		Query1<DescriptorPoolComponent>& descriptorPoolQuery = *queryBuilder->buildQuery<
			DescriptorPoolComponent>();
		Query1<PipelineLayoutComponent>& pipelineLayoutQuery = *queryBuilder->buildQuery<
			PipelineLayoutComponent>();
		Query1<PipelineComponent>& pipelineQuery = *queryBuilder->buildQuery<PipelineComponent>();

		Query1<UniformBufferComponent>& uniformBufferQuery = *queryBuilder->buildQuery<
			UniformBufferComponent>();

		SharedQuery1<UniformBufferSharedComponent>& uniformBufferSharedQuery = *queryBuilder->
			buildQuery<UniformBufferSharedComponent>();

		Query1<MeshComponent>& meshQuery = *queryBuilder->buildQuery<MeshComponent>();

		auto result = device.waitIdle();
		SASSERT_VULKAN(result)

		for (auto& mesh : meshQuery)
		{
			mesh.indexBuffer.destroy();
			mesh.vertexBuffer.destroy();
		}

		for (auto& ubo : uniformBufferQuery)
		{
			for (auto& buffer : ubo.ubos)
			{
				buffer.buffer.unmapMemory();
				buffer.buffer.destroy();
			}
		}

		for (auto& ubo : uniformBufferSharedQuery)
		{
			for (auto& buffer : ubo.ubos)
			{
				buffer.buffer.unmapMemory();
				buffer.buffer.destroy();
			}
		}

		auto* textureQueryPtr = m_entityService->queryBuilder()->buildQuery<TextureComponent>();
		auto& textureQuery = *textureQueryPtr;

		for (auto & texture : textureQuery)
		{
			gpuAllocatorComponent.allocator.destroyImage(texture.texture.image, texture.texture.allocation);
			device.destroyImageView(texture.imageView, allocationCallbacks);
			device.destroySampler(texture.sampler, allocationCallbacks);
		}


		for (auto& pipeline : pipelineQuery)
		{
			device.destroyPipeline(pipeline.pipeline, allocationCallbacks);
		}

		for (auto& pipelineLayout : pipelineLayoutQuery)
		{
			device.destroyPipelineLayout(pipelineLayout.layout, allocationCallbacks);
		}

		for (auto& pool : descriptorPoolQuery)
		{
			device.destroyDescriptorPool(pool.pool, allocationCallbacks);
		}

		for (auto& layout : descriptorLayoutQuery)
		{
			device.destroyDescriptorSetLayout(layout.layout, allocationCallbacks);
		}

		for (auto& shader : shaderQuery)
		{
			device.destroyShaderModule(shader.shaderModule, allocationCallbacks);
		}

		for (auto& semaphore : synchronizationComponent.imageAvailableSemaphores)
		{
			device.destroySemaphore(semaphore, allocationCallbacks);
		}
		for (auto& semaphore : synchronizationComponent.renderFinishedSemaphores)
		{
			device.destroySemaphore(semaphore, allocationCallbacks);
		}
		for (auto& fence : synchronizationComponent.inFlightFences)
		{
			device.destroyFence(fence, allocationCallbacks);
		}

		device.destroyCommandPool(commandPoolComponent.graphicsCommandPool, allocationCallbacks);
		device.destroyCommandPool(commandPoolComponent.transferCommandPool, allocationCallbacks);

		auto& framebufferQuery = *queryBuilder->buildQuery<FramebufferComponent>();

		for (const auto& fbComponent : framebufferQuery)
		{
			for (const auto& framebuffer : fbComponent.framebuffers)
			{
				device.destroyFramebuffer(framebuffer, allocationCallbacks);
			}
		}

		auto& renderPassQuery = *queryBuilder->buildQuery<RenderPassComponent>();

		for (const auto & renderPassComponent: renderPassQuery)
		{
			device.destroyRenderPass(renderPassComponent.renderPass, allocationCallbacks);
		}

		//auto& depthFbComponent = componentManager->singleton<DepthFramebufferComponent>();
		//auto& depthRpComponent = componentManager->singleton<DepthRenderPassComponent>();

		//for (auto& framebuffer : depthFbComponent.framebuffers)
		//{
		//	device.destroyFramebuffer(framebuffer, allocationCallbacks);
		//}
		//device.destroyRenderPass(depthRpComponent.renderPass, allocationCallbacks);

		//for (auto& framebuffer : framebufferComponent.framebuffers)
		//{
		//	device.destroyFramebuffer(framebuffer, allocationCallbacks);
		//}

		//device.destroyRenderPass(renderPass, allocationCallbacks);

		device.destroySwapchainKHR(swapchain, allocationCallbacks);

		for (auto& imageView : swapchainComponent.imageViews)
		{
			device.destroyImageView(imageView, allocationCallbacks);
		}

		device.destroySampler(componentManager->getSingleton<GBufferSampler>().sampler,allocationCallbacks);

		auto& gBuffer = componentManager->getSingleton<GBufferComponent>();
		gpuAllocatorComponent.allocator.destroyImage(gBuffer.positionImage.image, gBuffer.positionImage.allocation);
		device.destroyImageView(gBuffer.positionImageView, allocationCallbacks);
		gpuAllocatorComponent.allocator.destroyImage(gBuffer.normalsImage.image, gBuffer.normalsImage.allocation);
		device.destroyImageView(gBuffer.normalImageView, allocationCallbacks);
		gpuAllocatorComponent.allocator.destroyImage(gBuffer.albedoImage.image, gBuffer.albedoImage.allocation);
		device.destroyImageView(gBuffer.albedoImageView, allocationCallbacks);

		auto& depthImageComponent = componentManager->getSingleton<DepthImageComponent>();
		device.destroyImageView(depthImageComponent.imageView, allocationCallbacks);
		gpuAllocatorComponent.allocator.destroyImage(depthImageComponent.image.image,
		                                             depthImageComponent.image.allocation);

		gpuAllocatorComponent.allocator.destroy();
		device.destroy(allocationCallbacks);

		destroyDebugUtilsMessenger(instance,
		                           componentManager->getSingleton<DebugMessengerComponent>().messenger,
		                           nullptr);
		//instance.destroySurfaceKHR(surface,allocationCallbacks);
		instance.destroySurfaceKHR(surface);
		//instance.destroy();
		instance.destroy(allocationCallbacks);

		SDEBUG_LOG("CLEANUP COMPLETED")
		m_entityService->entityEventManager()->rewindEvent(typeid(CleanupRequest));
	}
}
