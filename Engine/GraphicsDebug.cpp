#include "GraphicsDebug.h"

VkResult createDebugUtilsMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroyDebugUtilsMessengerExt(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

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
