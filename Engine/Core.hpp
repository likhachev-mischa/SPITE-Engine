#pragma once
#include <EASTL/vector.h>

#include "Common.hpp"
#include "Application/AppConifg.hpp"
#include "Application/WindowManager.hpp"
#include "Base/Assert.hpp"
#include "Base/Memory.hpp"
#include "vulkan-memory-allocator-hpp/vk_mem_alloc.hpp"

namespace spite
{
	//TODO: replace most allocators to stack/linear allocator
#define SASSERT_VULKAN(result) SASSERTM((result) == vk::Result::eSuccess, "Vulkan assertion failed %u",result)

	const std::array<const char*, 1> DEVICE_EXTENSIONS = {
		vk::KHRSwapchainExtensionName
	};


	eastl::vector<const char*, spite::HeapAllocator> getRequiredExtensions(
		const spite::HeapAllocator& allocator, spite::WindowManager* windowManager);

	vk::Instance createInstance(const spite::HeapAllocator& allocator,
	                            const vk::AllocationCallbacks& allocationCallbacks,
	                            const eastl::vector<const char*, spite::HeapAllocator>& extensions);

	//TODO: implement allocation callbacks
	//allocation callbacks are unused for now

	vk::PhysicalDevice getPhysicalDevice(const vk::Instance& instance);

	QueueFamilyIndices findQueueFamilies(const vk::SurfaceKHR& surface, vk::PhysicalDevice& physicalDevice,
	                                     const spite::HeapAllocator& allocator);

	vk::Device createDevice(const QueueFamilyIndices& indices, const vk::PhysicalDevice& physicalDevice,
	                        const spite::HeapAllocator& allocator, const vk::AllocationCallbacks* pAllocationCallbacks);

	//TODO: add vma callbacks
	vma::Allocator createVmAllocator(const vk::PhysicalDevice& physicalDevice, const vk::Device& device,
	                                 const vk::Instance& instance, const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<vk::SurfaceFormatKHR>& availableFormats);

	vk::PresentModeKHR chooseSwapPresentMode(
		const std::vector<vk::PresentModeKHR>& availablePresentModes);

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, int width, int height);

	SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice physicalDevice,
	                                              const vk::SurfaceKHR surface);

	vk::SwapchainKHR createSwapchain(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface,
	                                 const SwapchainSupportDetails& swapchainSupport, const vk::Device& device,
	                                 const vk::Extent2D& extent, const vk::SurfaceFormatKHR& surfaceFormat,
	                                 const vk::PresentModeKHR& presentMode, const spite::HeapAllocator& allocator,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks);

	std::vector<vk::Image> getSwapchainImages(const vk::Device& device, const vk::SwapchainKHR& swapchain);

	eastl::vector<vk::ImageView, spite::HeapAllocator> createImageViews(const vk::Device& device,
	                                                                    const std::vector<vk::Image>& swapchainImages,
	                                                                    const vk::Format& imageFormat,
	                                                                    const vk::AllocationCallbacks*
	                                                                    pAllocationCallbacks);

	vk::RenderPass createRenderPass(const vk::Device& device, const vk::Format& imageFormat,
	                                const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::DescriptorSetLayout createDescriptorSetLayout(const vk::Device& device,
	                                                  const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::ShaderModule createShaderModule(const vk::Device& device, const std::vector<char>& code,
	                                    const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::Pipeline createGraphicsPipeline(const vk::Device& device, const vk::DescriptorSetLayout& descriptorSetLayout,
	                                    const vk::Extent2D& swapchainExtent, const vk::RenderPass& renderPass,
	                                    const vk::AllocationCallbacks* pAllocationCallbacks);

	eastl::vector<vk::Framebuffer, spite::HeapAllocator> createFramebuffers(const vk::Device& device,
	                                                                        const spite::HeapAllocator& allocator,
	                                                                        const eastl::vector<
		                                                                        vk::ImageView, spite::HeapAllocator>&
	                                                                        imageViews,
	                                                                        const vk::Extent2D& swapchainExtent,
	                                                                        const vk::RenderPass& renderPass,
	                                                                        const vk::AllocationCallbacks*
	                                                                        pAllocationCallbacks);

	vk::CommandPool createCommandPool(const vk::Device& device,
	                                  const vk::AllocationCallbacks* pAllocationCallbacks,
	                                  const vk::CommandPoolCreateFlags& flags, const u32 queueFamilyIndex);

	void createBuffer(const vk::DeviceSize& size,
	                  const vk::BufferUsageFlags& usage,
	                  const vk::MemoryPropertyFlags& properties,
	                  const vma::AllocationCreateFlags& allocationFlags,
	                  const QueueFamilyIndices& indices,
	                  const vma::Allocator& allocator,
	                  vk::Buffer& buffer,
	                  vma::Allocation& bufferMemory);

	vk::DescriptorPool createDescriptorPool(const vk::Device& device,
	                                        const vk::AllocationCallbacks* pAllocationCallbacks,
	                                        const vk::DescriptorType& type,
	                                        const u32 size = MAX_FRAMES_IN_FLIGHT);

	std::vector<vk::DescriptorSet> createDescriptorSets(const vk::Device& device,
	                                                    const vk::DescriptorSetLayout& descriptorSetLayout,
	                                                    const vk::DescriptorPool& descriptorPool, sizet,
	                                                    const spite::HeapAllocator& allocator,
	                                                    const vk::AllocationCallbacks* pAllocationCallbacks,
	                                                    const u32 count = MAX_FRAMES_IN_FLIGHT);

	void updateDescriptorSets(const vk::Device& device, const vk::DescriptorSet& descriptorSet,
	                          const vk::Buffer& buffer, const vk::DescriptorType& type, const sizet bufferElementSize);

	std::vector<vk::CommandBuffer> createGraphicsCommandBuffers(const vk::Device& device,
	                                                            const vk::CommandPool& graphicsCommandPool,
	                                                            const u32 count = MAX_FRAMES_IN_FLIGHT);

	vk::Semaphore createSemaphore(const vk::Device device, const vk::SemaphoreCreateInfo& createInfo,
	                              const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::Fence createFence(const vk::Device device, const vk::FenceCreateInfo& createInfo,
	                      const vk::AllocationCallbacks* pAllocationCallbacks);

	void recordCommandBuffer(const vk::CommandBuffer& commandBuffer, const vk::Extent2D& swapchainExtent,
	                         const vk::RenderPass& renderPass, const vk::Framebuffer& framebuffer,
	                         const vk::Pipeline& graphicsPipeline, const vk::Buffer& buffer,
	                         const vk::DeviceSize& indicesOffset, const vk::PipelineLayout& pipelineLayout,
	                         const vk::DescriptorSet& descriptorSet, const u32 indexCount);

	vk::Result waitForFrame(const vk::Device& device, const vk::SwapchainKHR swapchain, const vk::Fence& inFlightFence,
	                        const vk::Semaphore& imageAvaliableSemaphore, const vk::CommandBuffer& commandBuffer,
	                        u32& imageIndex);

	vk::Result drawFrame(const vk::CommandBuffer& commandBuffer, const vk::Fence& inFlightFence,
	                     const vk::Semaphore& imageAvaliableSemaphore, const vk::Semaphore& renderFinishedSemaphore,
	                     const vk::Queue& graphicsQueue, const vk::Queue& presentQueue,
	                     const vk::SwapchainKHR swapchain, const u32& imageIndex);
}
