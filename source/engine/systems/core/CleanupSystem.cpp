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

		SwapchainComponent swapchainComponent = componentManager->getSingleton<
			SwapchainComponent>();
		vk::SwapchainKHR swapchain = swapchainComponent.swapchain;
		vk::RenderPass renderPass = componentManager->getSingleton<MainRenderPassComponent>().
		                                              renderPass;
		MainFramebufferComponent& framebufferComponent = componentManager->getSingleton<
			MainFramebufferComponent>();
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

		auto& depthFbComponent = componentManager->getSingleton<DepthFramebufferComponent>();
		auto& depthRpComponent = componentManager->getSingleton<DepthRenderPassComponent>();

		for (auto& framebuffer : depthFbComponent.framebuffers)
		{
			device.destroyFramebuffer(framebuffer, allocationCallbacks);
		}
		device.destroyRenderPass(depthRpComponent.renderPass, allocationCallbacks);

		for (auto& framebuffer : framebufferComponent.framebuffers)
		{
			device.destroyFramebuffer(framebuffer, allocationCallbacks);
		}

		device.destroyRenderPass(renderPass, allocationCallbacks);

		device.destroySwapchainKHR(swapchain, allocationCallbacks);

		for (auto& imageView : swapchainComponent.imageViews)
		{
			device.destroyImageView(imageView, allocationCallbacks);
		}

		auto& depthImageComponent = componentManager->getSingleton<DepthImageComponent>();
		device.destroyImageView(depthImageComponent.imageView,allocationCallbacks);
		gpuAllocatorComponent.allocator.destroyImage(depthImageComponent.image, depthImageComponent.allocation);

		gpuAllocatorComponent.allocator.destroy();
		device.destroy(allocationCallbacks);

		destroyDebugUtilsMessenger(instance,
		                           componentManager->getSingleton<DebugMessengerComponent>().
		                                             messenger,
		                           nullptr);
		//instance.destroySurfaceKHR(surface,allocationCallbacks);
		instance.destroySurfaceKHR(surface);
		//instance.destroy();
		instance.destroy(allocationCallbacks);

		SDEBUG_LOG("CLEANUP COMPLETED")
		m_entityService->entityEventManager()->rewindEvent(typeid(CleanupRequest));
	}
}
