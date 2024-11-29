#pragma once
#include <array>
#include <iostream>
#include "VulkanUsage.hpp"
#include "Base/Memory.hpp"

namespace spite
{
#ifdef NDEBUG
	constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
	constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

	const std::array<const char*, 1> VALIDATION_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
	};

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage <<
			std::endl;

		return VK_FALSE;
	}

	VkResult createDebugUtilsMessengerExt(VkInstance instance,
	                                      const VkDebugUtilsMessengerCreateInfoEXT*
	                                      pCreateInfo,
	                                      const VkAllocationCallbacks* pAllocator,
	                                      VkDebugUtilsMessengerEXT* pDebugMessenger);

	void destroyDebugUtilsMessengerExt(VkInstance instance,
	                                   VkDebugUtilsMessengerEXT debugMessenger,
	                                   const VkAllocationCallbacks* pAllocator);

	vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();

	bool checkValidationLayerSupport(spite::HeapAllocator);
}
