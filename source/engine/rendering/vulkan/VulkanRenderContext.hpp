#pragma once
#include "base/Platform.hpp"
#include "base/VmaUsage.hpp"

#include "engine/rendering/IRenderContext.hpp"

namespace spite
{
	class VulkanWindowBinding;

	// This struct holds all the core, application-lifetime Vulkan objects.
	struct VulkanRenderContext : public IRenderContext
	{
		vk::Instance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		VmaAllocator allocator;

		vk::Queue graphicsQueue;
		u32 graphicsQueueFamily;
		vk::Queue presentQueue;
		u32 presentQueueFamily;
		vk::Queue transferQueue;
		u32 transferQueueFamily;

		VulkanRenderContext(VulkanWindowBinding* windowBinding);
		~VulkanRenderContext() override;

		VulkanRenderContext(const VulkanRenderContext&) = delete;
		VulkanRenderContext& operator=(const VulkanRenderContext&) = delete;
		VulkanRenderContext(VulkanRenderContext&&) = delete;
		VulkanRenderContext& operator=(VulkanRenderContext&&) = delete;

#ifndef SPITE_USE_DESCRIPTOR_SETS
		// --- Extension Function Pointers ---
		PFN_vkCmdBindDescriptorBuffersEXT pfnCmdBindDescriptorBuffersEXT;
		PFN_vkCmdSetDescriptorBufferOffsetsEXT pfnCmdSetDescriptorBufferOffsetsEXT;
		PFN_vkGetDescriptorEXT pfnGetDescriptorEXT;
		PFN_vkGetDescriptorSetLayoutSizeEXT pfnGetDescriptorSetLayoutSizeEXT;
		PFN_vkGetDescriptorSetLayoutBindingOffsetEXT pfnGetDescriptorSetLayoutBindingOffsetEXT;
#endif
	};
}
