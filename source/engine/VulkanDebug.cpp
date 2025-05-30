#include "VulkanDebug.hpp"

#include "Base/Assert.hpp"

#include "Engine/Common.hpp"

namespace spite
{
	vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo()
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo({},
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback);
		return createInfo;
	}

	bool checkValidationLayerSupport()
	{
		auto [result, availableLayers] = vk::enumerateInstanceLayerProperties();
		SASSERT_VULKAN(result)

			for (const char* layerName : VALIDATION_LAYERS)
			{
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
				{
					return false;
				}
			}

		return true;
	}

	//vk::DebugUtilsMessengerEXT createDebugUtilsMessenger(vk::Instance instance, const vk::AllocationCallbacks* allocationCallbacks)
	//{
	//	auto [result, messenger] = instance.createDebugUtilsMessengerEXT(createDebugMessengerCreateInfo(), allocationCallbacks);

	//	SASSERT_VULKAN(result);
	//	return messenger;
	//}

	VkResult createDebugUtilsMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
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
		const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo =
			createDebugMessengerCreateInfo();

		VkDebugUtilsMessengerEXT debugMessenger;

		auto result = createDebugUtilsMessengerExt(instance,
			&createInfo,
			nullptr,
			&debugMessenger);
		SASSERTM(result == VK_SUCCESS, "Failed to create debug messenger!")
			return debugMessenger;
	}

	void destroyDebugUtilsMessenger(const vk::Instance& instance, VkDebugUtilsMessengerEXT& debugMessenger,
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
}
