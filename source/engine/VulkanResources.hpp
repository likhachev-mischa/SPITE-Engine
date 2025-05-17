#pragma once
#include <EASTL/array.h>
#include <EASTL/vector.h>


#include "Application/AppConifg.hpp"

#include "Base/Memory.hpp"
#include "Base/VmaUsage.hpp"

#include "Engine/Common.hpp"

namespace spite
{
	struct VertexInputDescriptions
	{
		eastl::vector<vk::VertexInputBindingDescription, spite::HeapAllocator> bindingDescriptions;
		eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator>
		attributeDescriptions;

		VertexInputDescriptions(
			eastl::vector<vk::VertexInputBindingDescription, spite::HeapAllocator>
			bindingDescriptions,
			eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator>
			attributeDescriptions) : bindingDescriptions(std::move(bindingDescriptions)),
			                         attributeDescriptions(std::move(attributeDescriptions))
		{
		}
	};

	const eastl::array DEVICE_EXTENSIONS = {vk::KHRSwapchainExtensionName,vk::KHRSeparateDepthStencilLayoutsExtensionName,vk::KHRCreateRenderpass2ExtensionName };

	std::vector<const char*> getRequiredExtensions(char const* const* windowExtensions,
	                                               const u32 windowExtensionCount);

	vk::Instance createInstance(const vk::AllocationCallbacks& allocationCallbacks,
	                            const std::vector<const char*>& extensions);

	vk::PhysicalDevice getPhysicalDevice(const vk::Instance& instance);

	QueueFamilyIndices findQueueFamilies(const vk::SurfaceKHR& surface,
	                                     const vk::PhysicalDevice& physicalDevice);

	vk::Device createDevice(const QueueFamilyIndices& indices,
	                        const vk::PhysicalDevice& physicalDevice,
	                        const vk::AllocationCallbacks* pAllocationCallbacks);

	//TODO: add vma callbacks
	vma::Allocator createVmAllocator(const vk::PhysicalDevice& physicalDevice,
	                                 const vk::Device& device,
	                                 const vk::Instance& instance,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<vk::SurfaceFormatKHR>& availableFormats);

	vk::PresentModeKHR chooseSwapPresentMode(
		const std::vector<vk::PresentModeKHR>& availablePresentModes);

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
	                              int width,
	                              int height);

	SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice physicalDevice,
	                                              const vk::SurfaceKHR surface);

	vk::SwapchainKHR createSwapchain(const vk::Device& device,
	                                 const QueueFamilyIndices& indices,
	                                 const vk::SurfaceKHR& surface,
	                                 const vk::SurfaceCapabilitiesKHR& capabilities,
	                                 const vk::Extent2D& extent,
	                                 const vk::SurfaceFormatKHR& surfaceFormat,
	                                 const vk::PresentModeKHR& presentMode,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks);

	std::vector<vk::Image> getSwapchainImages(const vk::Device& device,
	                                          const vk::SwapchainKHR& swapchain);

	std::vector<vk::ImageView> createSwapchainImageViews(const vk::Device& device,
	                                                     const std::vector<vk::Image>&
	                                                     swapchainImages,
	                                                     const vk::Format& imageFormat,
	                                                     const vk::AllocationCallbacks*
	                                                     pAllocationCallbacks);

	struct DescriptorLayoutData
	{
		vk::DescriptorType type;
		u32 bindingIndex;
		vk::ShaderStageFlags shaderStages;
	};

	vk::DescriptorSetLayout createDescriptorSetLayout(const vk::Device& device,
	                                                  const std::vector<DescriptorLayoutData>&
	                                                  descriptorData,
	                                                  //const vk::DescriptorType& type,
	                                                  //const u32 bindingIndex,
	                                                  //const vk::ShaderStageFlags& stage,
	                                                  const vk::AllocationCallbacks*
	                                                  pAllocationCallbacks);

	vk::ShaderModule createShaderModule(const vk::Device& device,
	                                    const std::vector<char>& code,
	                                    const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::PipelineShaderStageCreateInfo createShaderStageInfo(const vk::Device& device,
	                                                        const std::vector<char>& code,
	                                                        const vk::ShaderStageFlagBits& stage,
	                                                        const char* name,
	                                                        const vk::AllocationCallbacks*
	                                                        pAllocationCallbacks);

	vk::PipelineLayout createPipelineLayout(const vk::Device& device,
	                                        const std::vector<vk::DescriptorSetLayout>&
	                                        descriptorSetLayouts,
	                                        const u32 pushConstantSize,
	                                        const vk::AllocationCallbacks* pAllocationCallbacks);


	vk::CommandPool createCommandPool(const vk::Device& device,
	                                  const vk::AllocationCallbacks* pAllocationCallbacks,
	                                  const vk::CommandPoolCreateFlags& flags,
	                                  const u32 queueFamilyIndex);

	//for graphics and transfer usage
	//TODO: parametrize
	void createBuffer(const vk::DeviceSize& size,
	                  const vk::BufferUsageFlags& usage,
	                  const vk::MemoryPropertyFlags& properties,
	                  const vma::AllocationCreateFlags& allocationFlags,
	                  const QueueFamilyIndices& indices,
	                  const vma::Allocator& allocator,
	                  vk::Buffer& buffer,
	                  vma::Allocation& bufferMemory);

	void copyBuffer(const vk::Buffer& srcBuffer,
	                const vk::Buffer& dstBuffer,
	                const vk::DeviceSize& size,
	                const vk::CommandPool& transferCommandPool,
	                const vk::Device& device,
	                const vk::Queue& transferQueue,
	                const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::DescriptorPool createDescriptorPool(const vk::Device& device,
	                                        const vk::AllocationCallbacks* pAllocationCallbacks,
	                                        const vk::DescriptorType& type,
	                                        const u32 size);

	std::vector<vk::DescriptorSet> createDescriptorSets(const vk::Device& device,
	                                                    const vk::DescriptorSetLayout&
	                                                    descriptorSetLayout,
	                                                    const vk::DescriptorPool& descriptorPool,
	                                                    const u32 count = MAX_FRAMES_IN_FLIGHT);

	void updateDescriptorSets(const vk::Device& device,
	                          const vk::DescriptorSet& descriptorSet,
	                          const vk::Buffer& buffer,
	                          const vk::DescriptorType& type,
	                          const u32 bindingIndex,
	                          const sizet bufferElementSize);

	std::vector<vk::CommandBuffer> createGraphicsCommandBuffers(const vk::Device& device,
	                                                            const vk::CommandPool&
	                                                            graphicsCommandPool,
	                                                            const vk::CommandBufferLevel& level,
	                                                            const u32 count =
		                                                            MAX_FRAMES_IN_FLIGHT);

	vk::Semaphore createSemaphore(const vk::Device device,
	                              const vk::SemaphoreCreateInfo& createInfo,
	                              const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::Fence createFence(const vk::Device device,
	                      const vk::FenceCreateInfo& createInfo,
	                      const vk::AllocationCallbacks* pAllocationCallbacks);
}
