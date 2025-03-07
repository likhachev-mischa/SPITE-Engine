#pragma once

#include "ecs/Core.hpp"
#include "ecs/Queries.hpp"
#include "ecs/World.hpp"

#include "engine/VulkanComponents.hpp"
#include "engine/ResourcesCore.hpp"
#include "Engine/VulkanAllocator.hpp"

namespace spite
{
	class VulkanInitSystem : public SystemBase
	{
	public:
		void onEnable() override
		{
			auto componentManager = m_entityService->componentManager();

			vk::AllocationCallbacks allocationCallbacks = vk::AllocationCallbacks(
				nullptr,
				&vkAllocate,
				&vkReallocate,
				&vkFree,
				nullptr, nullptr);

			AllocationCallbacksComponent allocationCallbacksComponent;
			allocationCallbacksComponent.allocationCallbacks = allocationCallbacks;

			componentManager->createSingleton(allocationCallbacksComponent);

			auto windowManager = componentManager->getSingleton<WindowManagerComponent>().windowManager;

			u32 windowExtensionsCount = 0;
			char const* const* windowExtensions = windowManager->getExtensions(windowExtensionsCount);
			std::vector<const char*> instanceExtensions =
				getRequiredExtensions(windowExtensions, windowExtensionsCount);

			vk::Instance instance = createInstance(allocationCallbacks, instanceExtensions);
			VulkanInstanceComponent instanceComponent;
			//instanceComponent.enabledExtensions = std::move(instanceExtensions);
			instanceComponent.instance = instance;
			componentManager->createSingleton(instanceComponent);

			vk::PhysicalDevice physicalDevice = getPhysicalDevice(instance);
			PhysicalDeviceComponent physicalDeviceComponent;
			physicalDeviceComponent.device = physicalDevice;
			componentManager->createSingleton(physicalDeviceComponent);


			vk::SurfaceKHR surface = windowManager->createWindowSurface(instance);
			SurfaceComponent surfaceComponent;
			surfaceComponent.surface = surface;
			SwapchainSupportDetails swapchainSupportDetails = querySwapchainSupport(physicalDevice, surface);
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

			vma::Allocator gpuAllocator = createVmAllocator(physicalDevice, device, instance, &allocationCallbacks);
			GpuAllocatorComponent gpuAllocatorComponent;
			gpuAllocatorComponent.allocator = gpuAllocator;
			componentManager->createSingleton(gpuAllocatorComponent);

			//TODO: replace constants with config
			vk::Extent2D swapExtent = chooseSwapExtent(surfaceComponent.capabilities, WIDTH, HEIGHT);

			vk::SwapchainKHR swapchain = createSwapchain(device, queueFamilyIndices, surface,
			                                             surfaceComponent.capabilities, swapExtent,
			                                             surfaceComponent.surfaceFormat, surfaceComponent.presentMode,
			                                             &allocationCallbacks);
			SwapchainComponent swapchainComponent;
			swapchainComponent.swapchain = swapchain;
			swapchainComponent.imageFormat = surfaceComponent.surfaceFormat.format;
			swapchainComponent.extent = swapExtent;
			swapchainComponent.images = getSwapchainImages(device, swapchain);
			swapchainComponent.imageViews = createImageViews(device, swapchainComponent.images,
			                                                 swapchainComponent.imageFormat, &allocationCallbacks);

			requireComponent(typeid(VulkanInitRequest));
		}


	};
}
