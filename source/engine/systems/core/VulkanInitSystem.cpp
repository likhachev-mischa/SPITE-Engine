#include "CoreSystems.hpp"

#include "engine/VulkanAllocator.hpp"
#include "engine/VulkanDebug.hpp"
#include "application/WindowManager.hpp"

#include "engine/VulkanImages.hpp"

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

		auto windowManager = componentManager->getSingleton<WindowManagerComponent>().windowManager;

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
		swapchainComponent.imageViews = createImageViews(device,
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

		componentManager->createSingleton(std::move(cbComponent));


		//DEPTH IMAGE
		Image depthImage = createDepthImage(queueFamilyIndices, swapExtent, gpuAllocator);
		vk::ImageView depthImageView = createDepthImageView(device, depthImage.image, allocationCallbacks);

		DepthImageComponent depthImageComponent;
		depthImageComponent.image = depthImage.image;
		depthImageComponent.allocation = depthImage.allocation;
		depthImageComponent.imageView = depthImageView;

		componentManager->createSingleton(depthImageComponent);

		//TODO: smart pipeline creation
		RenderPassComponent renderPassComponent;
		renderPassComponent.renderPass = createRenderPass(device,
		                                                  swapchainComponent.imageFormat,
		                                                  &allocationCallbacks);
		componentManager->createSingleton(renderPassComponent);

		FramebufferComponent framebufferComponent;
		framebufferComponent.framebuffers = createFramebuffers(
			device,
			swapchainComponent.imageViews,
			depthImageView,
			swapExtent,
			renderPassComponent.renderPass,
			&allocationCallbacks);
		componentManager->createSingleton(std::move(framebufferComponent));

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
