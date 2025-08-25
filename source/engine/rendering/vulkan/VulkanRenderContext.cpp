#include "VulkanRenderContext.hpp"

#include <optional>
#include <set>

#include <EASTL/array.h>

#include "application/vulkan/VulkanWindowBinding.hpp"

#include "base/CollectionUtilities.hpp"

namespace spite
{
	namespace
	{
#ifdef DEBUG
		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			vk::DebugUtilsMessageTypeFlagsEXT messageType,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
		{
			SDEBUG_LOG("validation layer: %s\n", pCallbackData->pMessage)
			SASSERT(messageSeverity != vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
			return VK_FALSE;
		}

		VkResult createDebugUtilsMessengerExt(VkInstance instance,
		                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		                                      const VkAllocationCallbacks* pAllocator,
		                                      VkDebugUtilsMessengerEXT* pDebugMessenger)
		{
			auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
				instance,
				"vkCreateDebugUtilsMessengerEXT"));
			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		VkDebugUtilsMessengerEXT createDebugUtilsMessenger(const vk::Instance& instance,
		                                                   const VkDebugUtilsMessengerCreateInfoEXT& createInfo,
		                                                   const vk::AllocationCallbacks* pAllocationCallbacks)
		{
			VkDebugUtilsMessengerEXT debugMessenger;

			auto result = createDebugUtilsMessengerExt(instance,
			                                           &createInfo,
			                                           nullptr,
			                                           &debugMessenger);
			SASSERTM(result == VK_SUCCESS, "Failed to create debug messenger!")
			return debugMessenger;
		}

		void destroyDebugUtilsMessenger(const vk::Instance& instance, const VkDebugUtilsMessengerEXT& debugMessenger,
		                                const VkAllocationCallbacks* pAllocator)
		{
			auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
				instance,
				"vkDestroyDebugUtilsMessengerEXT"));
			if (func != nullptr)
			{
				func(instance, debugMessenger, pAllocator);
			}
		}

#endif

		// Helper to find queue families
		struct QueueFamilyIndices
		{
			std::optional<u32> graphicsFamily;
			std::optional<u32> presentFamily;
			std::optional<u32> transferFamily;

			bool isComplete() const
			{
				return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
			}
		};

		QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
		{
			QueueFamilyIndices indices;
			auto queueFamilies = device.getQueueFamilyProperties();
			int i = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
				{
					indices.graphicsFamily = i;
				}

				// Look for a dedicated transfer queue
				if ((queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) && !(queueFamily.queueFlags &
					vk::QueueFlagBits::eGraphics))
				{
					indices.transferFamily = i;
				}

				auto presentSupport = device.getSurfaceSupportKHR(i, surface);
				if (presentSupport.value)
				{
					indices.presentFamily = i;
				}

				if (indices.isComplete())
				{
					break;
				}
				i++;
			}

			// If no dedicated transfer queue is found, fall back to the graphics queue
			if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value())
			{
				indices.transferFamily = indices.graphicsFamily;
			}

			return indices;
		}

		namespace
		{
			constexpr eastl::array deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_NESTED_COMMAND_BUFFER_EXTENSION_NAME,
				VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
#ifndef  SPITE_USE_DESCRIPTOR_SETS
				VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME
#endif
			};
		}

		bool checkDeviceExtensionSupport(vk::PhysicalDevice device)
		{
			auto [res, availableExtensions] = device.enumerateDeviceExtensionProperties();
			SASSERT_VULKAN(res)

			for (const char* extension : deviceExtensions)
			{
				bool found = false;
				for (const auto& availableExtension : availableExtensions)
				{
					if (strcmp(extension, availableExtension.extensionName) == 0)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					SDEBUG_LOG("	- Missing required extension: %s\n", extension)
					return false;
				}
			}
			return true;
		}

		vk::PhysicalDevice selectPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
		{
			SDEBUG_LOG("Selecting a GPU...\n")

			auto resDevices = instance.enumeratePhysicalDevices();

			SASSERT_VULKAN(resDevices.result)
			auto& devices = resDevices.value;
			SASSERTM(!devices.empty(), "Failed to find GPUs with Vulkan support!")

			vk::PhysicalDevice bestDevice = nullptr;
			u32 bestScore = 0;

			SDEBUG_LOG("Available GPUs:\n")
			for (const auto& device : devices)
			{
				vk::PhysicalDeviceProperties properties = device.getProperties();
				SDEBUG_LOG("Evaluating device: %s\n", properties.deviceName.data())

				QueueFamilyIndices indices = findQueueFamilies(device, surface);
				bool extensionsSupported = checkDeviceExtensionSupport(device);
				if (!indices.isComplete() || !extensionsSupported)
				{
					if (!indices.isComplete())
						SDEBUG_LOG("Device's queue families incomplete.\n")
					if (!extensionsSupported)
						SDEBUG_LOG("Device's : Required device extensions not supported.\n")
					continue;
				}

				u32 currentScore = 0;

				// Score based on device type
				if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) currentScore += 1000;
				if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) currentScore += 500;

				// Score based on VRAM size
				vk::PhysicalDeviceMemoryProperties memProperties = device.getMemoryProperties();
				for (u32 i = 0; i < memProperties.memoryHeapCount; ++i)
				{
					if (memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
					{
						currentScore += static_cast<u32>(memProperties.memoryHeaps[i].size / MB);
						// Score per MB
					}
				}

				SDEBUG_LOG("  - %s (Score: %u)\n", properties.deviceName.data(), currentScore)

				if (currentScore > bestScore)
				{
					bestScore = currentScore;
					bestDevice = device;
				}
			}

			SASSERTM(bestDevice, "Failed to find a suitable GPU!\n")
			if (bestDevice)
			{
				vk::PhysicalDeviceProperties props = bestDevice.getProperties();
				SDEBUG_LOG("Selected GPU: %s\n", props.deviceName.data())
			}
			return bestDevice;
		}
	}

	VulkanRenderContext::VulkanRenderContext(VulkanWindowBinding* windowBinding)
	{
		vk::ApplicationInfo appInfo("SPITE", 1, "SPITE Engine", 1, VK_API_VERSION_1_4);

		vk::InstanceCreateInfo createInfo{};
		createInfo.pApplicationInfo = &appInfo;

		u32 extensionsCount;
		auto windowExtensions = windowBinding->getRequiredInstanceExtensions(extensionsCount);

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto extensions = makeScratchVector<cstring>(FrameScratchAllocator::get());
		extensions.reserve(extensionsCount);

		for (u32 i = 0; i < extensionsCount; ++i)
		{
			extensions.push_back(windowExtensions[i]);
		}

		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef _DEBUG
		vk::ValidationFeaturesEXT validationFeatures;
		auto enabledFeat = vk::ValidationFeatureEnableEXT::eDebugPrintf;
		validationFeatures.enabledValidationFeatureCount = 1;
		validationFeatures.pEnabledValidationFeatures = &enabledFeat;
		createInfo.pNext = &validationFeatures;

		constexpr eastl::array<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
		createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
#else
        createInfo.enabledLayerCount = 0;
#endif

		auto resInstance = vk::createInstance(createInfo);
		SASSERT_VULKAN(resInstance.result)
		instance = resInstance.value;

		surface = windowBinding->createSurface(instance);

		//DebugMessenger
#ifdef _DEBUG
		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
		debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		debugCreateInfo.pfnUserCallback = debugCallback;

		auto messenger = createDebugUtilsMessenger(instance, debugCreateInfo, nullptr);

		debugMessenger = messenger;
#endif

		// --- Select Physical Device ---
		physicalDevice = selectPhysicalDevice(instance, surface);

		// --- Create Logical Device ---
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::set<u32> uniqueQueueFamilies = {
			indices.graphicsFamily.value(),
			indices.presentFamily.value(),
			indices.transferFamily.value()
		};

		float queuePriority = 1.0f;
		for (u32 queueFamily : uniqueQueueFamilies)
		{
			vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamily, 1, &queuePriority);
			queueCreateInfos.push_back(queueCreateInfo);
		}

		vk::PhysicalDeviceNestedCommandBufferFeaturesEXT nestedCbFeatures{};
		nestedCbFeatures.nestedCommandBuffer = true;

		vk::PhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeatures{};
		descriptorBufferFeatures.pNext = &nestedCbFeatures;

#if defined (SPITE_USE_DESCRIPTOR_SETS)
		descriptorBufferFeatures.descriptorBuffer = VK_FALSE;
#else
		descriptorBufferFeatures.descriptorBuffer = VK_TRUE;
#endif

		vk::PhysicalDeviceVulkan14Features features14 = {};
		features14.pNext = &descriptorBufferFeatures;

		vk::PhysicalDeviceVulkan13Features features13 = {};
		features13.pNext = &features14;
		features13.dynamicRendering = VK_TRUE;
		features13.synchronization2 = VK_TRUE;

		vk::PhysicalDeviceVulkan12Features features12 = {};
		features12.pNext = &features13;
		features12.bufferDeviceAddress = VK_TRUE;
		features12.separateDepthStencilLayouts = VK_TRUE;
		features12.descriptorIndexing = VK_TRUE;

		vk::PhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.pNext = &features12;
		deviceFeatures2.features.samplerAnisotropy = VK_TRUE;

		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.pNext = &deviceFeatures2;
		deviceCreateInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = nullptr;

		deviceCreateInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();


		auto [deviceResult, createdDevice] = physicalDevice.createDevice(deviceCreateInfo);
		SASSERT_VULKAN(deviceResult)
		device = createdDevice;

		graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
		presentQueue = device.getQueue(indices.presentFamily.value(), 0);
		transferQueue = device.getQueue(indices.transferFamily.value(), 0);

		graphicsQueueFamily = indices.graphicsFamily.value();
		presentQueueFamily = indices.presentFamily.value();
		transferQueueFamily = indices.transferFamily.value();

		// --- Create VMA Allocator ---
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;
		vmaCreateAllocator(&allocatorInfo, &allocator);

#ifndef SPITE_USE_DESCRIPTOR_SETS
		// Load extension function pointers
		pfnCmdBindDescriptorBuffersEXT = reinterpret_cast<PFN_vkCmdBindDescriptorBuffersEXT>(device.getProcAddr(
			"vkCmdBindDescriptorBuffersEXT"));
		pfnCmdSetDescriptorBufferOffsetsEXT = reinterpret_cast<PFN_vkCmdSetDescriptorBufferOffsetsEXT>(device.
			getProcAddr("vkCmdSetDescriptorBufferOffsetsEXT"));
		pfnGetDescriptorEXT = reinterpret_cast<PFN_vkGetDescriptorEXT>(device.getProcAddr("vkGetDescriptorEXT"));
		pfnGetDescriptorSetLayoutSizeEXT = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSizeEXT>(device.getProcAddr(
			"vkGetDescriptorSetLayoutSizeEXT"));
		pfnGetDescriptorSetLayoutBindingOffsetEXT = reinterpret_cast<PFN_vkGetDescriptorSetLayoutBindingOffsetEXT>(
			device.getProcAddr("vkGetDescriptorSetLayoutBindingOffsetEXT"));

		SASSERTM(
			pfnCmdBindDescriptorBuffersEXT && pfnCmdSetDescriptorBufferOffsetsEXT && pfnGetDescriptorEXT &&
			pfnGetDescriptorSetLayoutSizeEXT && pfnGetDescriptorSetLayoutBindingOffsetEXT,
			"Failed to load descriptor buffer extension function pointers.")
#endif
	}

	VulkanRenderContext::~VulkanRenderContext()
	{
		if (allocator)
		{
			vmaDestroyAllocator(allocator);
		}
		if (device)
		{
			device.destroy();
		}
#ifdef _DEBUG
		if (debugMessenger)
		{
			destroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
		}
#endif
		if (surface)
		{
			instance.destroySurfaceKHR(surface);
		}
		if (instance)
		{
			instance.destroy();
		}
	}
}
