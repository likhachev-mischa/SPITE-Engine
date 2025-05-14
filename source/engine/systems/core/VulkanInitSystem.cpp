#include "CoreSystems.hpp"

#include "application/WindowManager.hpp"

#include "base/Common.hpp"
#include "base/File.hpp"

#include "engine/VulkanAllocator.hpp"
#include "engine/VulkanDebug.hpp"
#include "engine/VulkanDepth.hpp"
#include "engine/VulkanGeometry.hpp"
#include "engine/VulkanImages.hpp"
#include "engine/VulkanResources.hpp"

namespace spite
{
	void VulkanInitSystem::onInitialize()
	{
		auto componentManager = m_entityService->componentManager();

		vk::AllocationCallbacks allocationCallbacks = vk::AllocationCallbacks(
			m_entityService->allocatorPtr(),
			&vkAllocate,
			&vkReallocate,
			&vkFree,
			nullptr,
			nullptr);

		AllocationCallbacksComponent allocationCallbacksComponent;
		allocationCallbacksComponent.allocationCallbacks = allocationCallbacks;

		componentManager->createSingleton(allocationCallbacksComponent);

		auto windowManager = componentManager->singleton<WindowManagerComponent>().windowManager;

		u32 windowExtensionsCount = 0;
		char const* const* windowExtensions = windowManager->getExtensions(windowExtensionsCount);
		std::vector<const char*> instanceExtensions = getRequiredExtensions(
			windowExtensions,
			windowExtensionsCount);

		vk::Instance instance = createInstance(allocationCallbacks, instanceExtensions);
		VulkanInstanceComponent instanceComponent;
		//instanceComponent.enabledExtensions = std::move(instanceExtensions);
		instanceComponent.instance = instance;
		componentManager->createSingleton(instanceComponent);

		//debug messenger
		DebugMessengerComponent debugMessenger;
		debugMessenger.messenger = createDebugUtilsMessenger(instance, &allocationCallbacks);
		componentManager->createSingleton(debugMessenger);

		vk::PhysicalDevice physicalDevice = getPhysicalDevice(instance);
		PhysicalDeviceComponent physicalDeviceComponent;
		physicalDeviceComponent.device = physicalDevice;
		physicalDeviceComponent.properties = physicalDevice.getProperties();
		physicalDeviceComponent.features = physicalDevice.getFeatures();
		physicalDeviceComponent.memoryProperties = physicalDevice.getMemoryProperties();
		componentManager->createSingleton(physicalDeviceComponent);


		vk::SurfaceKHR surface = windowManager->createWindowSurface(instance);
		SurfaceComponent surfaceComponent;
		surfaceComponent.surface = surface;
		SwapchainSupportDetails swapchainSupportDetails = querySwapchainSupport(
			physicalDevice,
			surface);
		surfaceComponent.capabilities = swapchainSupportDetails.capabilities;
		surfaceComponent.presentMode = chooseSwapPresentMode(swapchainSupportDetails.presentModes);
		surfaceComponent.surfaceFormat = chooseSwapSurfaceFormat(swapchainSupportDetails.formats);
		componentManager->createSingleton(surfaceComponent);

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(surface, physicalDevice);
		vk::Device device = createDevice(queueFamilyIndices, physicalDevice, &allocationCallbacks);
		DeviceComponent deviceComponent;
		deviceComponent.device = device;
		deviceComponent.queueFamilyIndices = queueFamilyIndices;
		componentManager->createSingleton(deviceComponent);

		QueueComponent queueComponent;
		queueComponent.graphicsQueue = device.getQueue(queueFamilyIndices.graphicsFamily.value(),
		                                               queueComponent.graphicsQueueIndex);
		queueComponent.presentQueue = device.getQueue(queueFamilyIndices.presentFamily.value(),
		                                              queueComponent.presentQueueIndex);
		queueComponent.transferQueue = device.getQueue(queueFamilyIndices.transferFamily.value(),
		                                               queueComponent.transferQueueIndex);
		componentManager->createSingleton(queueComponent);

		vma::Allocator gpuAllocator = createVmAllocator(physicalDevice,
		                                                device,
		                                                instance,
		                                                &allocationCallbacks);
		GpuAllocatorComponent gpuAllocatorComponent;
		gpuAllocatorComponent.allocator = gpuAllocator;
		componentManager->createSingleton(gpuAllocatorComponent);

		FrameDataComponent frameData;
		componentManager->createSingleton(frameData);

		//TODO: replace constants with config
		vk::Extent2D swapExtent = chooseSwapExtent(surfaceComponent.capabilities, WIDTH, HEIGHT);

		vk::SwapchainKHR swapchain = createSwapchain(device,
		                                             queueFamilyIndices,
		                                             surface,
		                                             surfaceComponent.capabilities,
		                                             swapExtent,
		                                             surfaceComponent.surfaceFormat,
		                                             surfaceComponent.presentMode,
		                                             &allocationCallbacks);
		SwapchainComponent swapchainComponent;
		swapchainComponent.swapchain = swapchain;
		swapchainComponent.imageFormat = surfaceComponent.surfaceFormat.format;
		swapchainComponent.extent = swapExtent;
		swapchainComponent.images = getSwapchainImages(device, swapchain);
		swapchainComponent.imageViews = createSwapchainImageViews(device,
			swapchainComponent.images,
			swapchainComponent.imageFormat,
			&allocationCallbacks);
		componentManager->createSingleton(swapchainComponent);

		CommandPoolComponent commandPoolComponent;
		commandPoolComponent.graphicsCommandPool = createCommandPool(
			device,
			&allocationCallbacks,
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			queueFamilyIndices.graphicsFamily.value());
		commandPoolComponent.transferCommandPool = createCommandPool(
			device,
			&allocationCallbacks,
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			queueFamilyIndices.transferFamily.value());
		componentManager->createSingleton(commandPoolComponent);

		CommandBufferComponent cbComponent;
		//cbComponent.primaryBuffers = createGraphicsCommandBuffers(
		//	device,
		//	commandPoolComponent.graphicsCommandPool,
		//	vk::CommandBufferLevel::ePrimary,
		//	MAX_FRAMES_IN_FLIGHT);
		//cbComponent.secondaryBuffers = createGraphicsCommandBuffers(
		//	device,
		//	commandPoolComponent.graphicsCommandPool,
		//	vk::CommandBufferLevel::eSecondary,
		//	MAX_FRAMES_IN_FLIGHT);

		std::copy_n(
			createGraphicsCommandBuffers(device,
			                             commandPoolComponent.graphicsCommandPool,
			                             vk::CommandBufferLevel::ePrimary,
			                             MAX_FRAMES_IN_FLIGHT).begin(),
			MAX_FRAMES_IN_FLIGHT,
			cbComponent.primaryBuffers.begin());
		std::copy_n(
			createGraphicsCommandBuffers(device,
			                             commandPoolComponent.graphicsCommandPool,
			                             vk::CommandBufferLevel::eSecondary,
			                             MAX_FRAMES_IN_FLIGHT).begin(),
			MAX_FRAMES_IN_FLIGHT,
			cbComponent.secondaryBuffers.begin());
		std::copy_n(
			createGraphicsCommandBuffers(device,
			                             commandPoolComponent.graphicsCommandPool,
			                             vk::CommandBufferLevel::eSecondary,
			                             MAX_FRAMES_IN_FLIGHT).begin(),
			MAX_FRAMES_IN_FLIGHT,
			cbComponent.depthBuffers.begin());

		componentManager->createSingleton(std::move(cbComponent));


		//DEPTH IMAGE
		Image depthImage = createImage({swapExtent.width, swapExtent.height, 1},
		                               vk::Format::eD32Sfloat,
		                               vk::ImageUsageFlagBits::eDepthStencilAttachment,
		                               gpuAllocator);
		vk::ImageView depthImageView = createImageView(device,
		                                               depthImage,
		                                               vk::ImageAspectFlagBits::eDepth,
		                                               allocationCallbacks);

		DepthImageComponent depthImageComponent;
		depthImageComponent.image = depthImage;

		depthImageComponent.imageView = depthImageView;

		componentManager->createSingleton(depthImageComponent);

		//TODO: smart pipeline creation
		GeometryRenderPassComponent renderPassComponent;
		renderPassComponent.renderPass = createGeometryRenderPass(device,
			swapchainComponent.imageFormat,
			&allocationCallbacks);
		componentManager->createSingleton(renderPassComponent);

		GeometryFramebufferComponent framebufferComponent;
		framebufferComponent.framebuffers = createGeometryFramebuffers(
			device,
			swapchainComponent.imageViews,
			depthImageView,
			swapExtent,
			renderPassComponent.renderPass,
			&allocationCallbacks);
		componentManager->createSingleton(std::move(framebufferComponent));

		//TEMPORARY
		//depth
		DepthRenderPassComponent depthRenderPassComponent;
		depthRenderPassComponent.renderPass = createDepthRenderPass(device, &allocationCallbacks);
		componentManager->createSingleton(depthRenderPassComponent);

		DepthFramebufferComponent depthFramebufferComponent;
		depthFramebufferComponent.framebuffers = createDepthFramebuffers(
			swapchainComponent.imageViews.size(),
			device,
			depthImageView,
			swapExtent,
			depthRenderPassComponent.renderPass,
			&allocationCallbacks);
		componentManager->createSingleton(depthFramebufferComponent);

		Entity depthShaderEntity = m_entityService->entityManager()->createEntity();
		ShaderComponent depthShader;
		depthShader.filePath = "./shaders/depth.spv";
		depthShader.stage = vk::ShaderStageFlagBits::eVertex;
		depthShader.shaderModule = createShaderModule(device,
		                                              readBinaryFile(depthShader.filePath.c_str()),
		                                              &allocationCallbacks);
		componentManager->addComponent(depthShaderEntity, depthShader);

		auto depthDescriptorSetLayout = createDescriptorSetLayout(
			device,
			vk::DescriptorType::eUniformBuffer,
			0,
			vk::ShaderStageFlagBits::eVertex,
			&allocationCallbacks);
		auto depthDescriptorPool = createDescriptorPool(device,
		                                                &allocationCallbacks,
		                                                vk::DescriptorType::eUniformBuffer,
		                                                MAX_FRAMES_IN_FLIGHT);
		auto depthDescriptorSets = createDescriptorSets(device,
		                                                depthDescriptorSetLayout,
		                                                depthDescriptorPool,
		                                                MAX_FRAMES_IN_FLIGHT);

		auto depthPipelineLayout = createPipelineLayout(device,
		                                                {depthDescriptorSetLayout},
		                                                sizeof(glm::mat4),
		                                                &allocationCallbacks);
		Entity depthDescriptorEntity = m_entityService->entityManager()->createEntity();
		DescriptorPoolComponent depthDescriptorPoolComponent;
		depthDescriptorPoolComponent.maxSets = MAX_FRAMES_IN_FLIGHT;
		depthDescriptorPoolComponent.pool = depthDescriptorPool;
		componentManager->addComponent(depthDescriptorEntity, depthDescriptorPoolComponent);
		DescriptorSetLayoutComponent depthDescriptorSetLayoutComponent;
		depthDescriptorSetLayoutComponent.bindingIndex = 0;
		depthDescriptorSetLayoutComponent.layout = depthDescriptorSetLayout;
		depthDescriptorSetLayoutComponent.stages = vk::ShaderStageFlagBits::eVertex;
		depthDescriptorSetLayoutComponent.type = vk::DescriptorType::eUniformBuffer;
		componentManager->addComponent(depthDescriptorEntity, depthDescriptorSetLayoutComponent);
		DescriptorSetsComponent depthDescriptorSetsComponent;
		std::copy_n(depthDescriptorSets.begin(),
		            MAX_FRAMES_IN_FLIGHT,
		            depthDescriptorSetsComponent.descriptorSets.begin());
		componentManager->addComponent(depthDescriptorEntity, depthDescriptorSetsComponent);

		Entity depthPipelineLayoutEntity = m_entityService->entityManager()->createEntity();
		PipelineLayoutComponent depthPipelineLayoutComponent;
		depthPipelineLayoutComponent.descriptorSetLayoutEntities = {depthDescriptorEntity};
		depthPipelineLayoutComponent.layout = depthPipelineLayout;
		componentManager->addComponent(depthPipelineLayoutEntity, depthPipelineLayoutComponent);

		VertexInputData vertexInputData;

		vertexInputData.bindingDescriptions.push_back({0, sizeof(Vertex)});

		vertexInputData.attributeDescriptions.reserve(2);
		vertexInputData.attributeDescriptions.push_back({0, 0, vk::Format::eR32G32B32Sfloat});
		vertexInputData.attributeDescriptions.push_back({1, 0, vk::Format::eR32G32B32Sfloat});
		vk::PipelineVertexInputStateCreateInfo depthVertexInputInfo(
			{},
			1,
			vertexInputData.bindingDescriptions.data(),
			2,
			vertexInputData.attributeDescriptions.data());

		vk::PipelineShaderStageCreateInfo depthShaderStageCreateInfo(
			{},
			vk::ShaderStageFlagBits::eVertex,
			depthShader.shaderModule,
			"main");
		auto depthPipeline = createDepthPipeline(device,
		                                         depthPipelineLayout,
		                                         swapExtent,
		                                         depthRenderPassComponent.renderPass,
		                                         {depthShaderStageCreateInfo},
		                                         depthVertexInputInfo,
		                                         &allocationCallbacks);

		Entity depthPipelineEntity = m_entityService->entityManager()->
		                                              createEntity("DepthPipeline");
		PipelineComponent depthPipelineComponent;
		depthPipelineComponent.vertexInputData = vertexInputData;
		depthPipelineComponent.pipeline = depthPipeline;
		depthPipelineComponent.pipelineLayoutEntity = depthPipelineLayoutEntity;
		componentManager->addComponent(depthPipelineEntity, depthPipelineComponent);
		componentManager->addComponent<DepthPipelineTag>(depthPipelineEntity);

		//


		std::vector<vk::Semaphore> imageAvailableSemaphores;
		imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
		std::vector<vk::Semaphore> renderFinishedSemaphores;
		renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
		std::vector<vk::Fence> inFlightFences;
		inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
		for (sizet i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			imageAvailableSemaphores.push_back(createSemaphore(device, {}, &allocationCallbacks));
			renderFinishedSemaphores.push_back(createSemaphore(device, {}, &allocationCallbacks));
			inFlightFences.push_back(createFence(device,
			                                     {vk::FenceCreateFlagBits::eSignaled},
			                                     &allocationCallbacks));
		}

		SynchronizationComponent synchronizationComponent;
		//		synchronizationComponent.imageAvailableSemaphores = std::move(imageAvailableSemaphores);
		//		synchronizationComponent.renderFinishedSemaphores = std::move(renderFinishedSemaphores);
		//		synchronizationComponent.inFlightFences = std::move(inFlightFences);
		std::copy_n(imageAvailableSemaphores.begin(),
		            MAX_FRAMES_IN_FLIGHT,
		            synchronizationComponent.imageAvailableSemaphores.begin());
		std::copy_n(renderFinishedSemaphores.begin(),
		            MAX_FRAMES_IN_FLIGHT,
		            synchronizationComponent.renderFinishedSemaphores.begin());
		std::copy_n(inFlightFences.begin(),
		            MAX_FRAMES_IN_FLIGHT,
		            synchronizationComponent.inFlightFences.begin());

		componentManager->createSingleton(std::move(synchronizationComponent));

		SDEBUG_LOG("VULKAN INITIALIZED\n")
		//	requireComponent(typeid(VulkanInitRequest));
	}
}
