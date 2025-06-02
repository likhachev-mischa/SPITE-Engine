#pragma once
//TODO: refactor
#include "Base/Logging.hpp"
#include "Base/Memory.hpp"
#include "Base/VulkanUsage.hpp"

namespace spite
{
#ifdef DEBUG
	constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
	constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

	const std::array VALIDATION_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
	};

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		SDEBUG_LOG("validation layer: %s", pCallbackData->pMessage)
		SDEBUG_LOG("\n")
		return VK_FALSE;
	}

	vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo();

	bool checkValidationLayerSupport();


	//TODO: implement allocator
	//allocator is unused for now
	VkResult createDebugUtilsMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	                                      const VkAllocationCallbacks* pAllocator,
	                                      VkDebugUtilsMessengerEXT* pDebugMessenger);

	//vk::DebugUtilsMessengerEXT createDebugUtilsMessenger(vk::Instance instance, const vk::AllocationCallbacks* allocationCallbacks);
	VkDebugUtilsMessengerEXT createDebugUtilsMessenger(const vk::Instance& instance,
	                                                   const vk::AllocationCallbacks* pAllocationCallbacks);

	void destroyDebugUtilsMessenger(const vk::Instance& instance, VkDebugUtilsMessengerEXT& debugMessenger,
	                                const VkAllocationCallbacks* pAllocator);
}
