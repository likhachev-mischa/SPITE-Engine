#pragma once
#include "Base/Platform.hpp"
#include "Base/VulkanUsage.hpp"

namespace spite
{
	void recordCommandBuffer(const vk::CommandBuffer& commandBuffer, const vk::Extent2D& swapchainExtent,
	                         const vk::RenderPass& renderPass, const vk::Framebuffer& framebuffer,
	                         const vk::Pipeline& graphicsPipeline, const vk::Buffer& buffer,
	                         const vk::DeviceSize& indicesOffset, const vk::PipelineLayout& pipelineLayout,
	                         const vk::DescriptorSet& descriptorSet, const u32 indicesCount);

	vk::Result waitForFrame(const vk::Device& device, const vk::SwapchainKHR swapchain, const vk::Fence& inFlightFence,
	                        const vk::Semaphore& imageAvaliableSemaphore, const vk::CommandBuffer& commandBuffer,
	                        u32& imageIndex);

	vk::Result drawFrame(const vk::CommandBuffer& commandBuffer, const vk::Fence& inFlightFence,
	                     const vk::Semaphore& imageAvaliableSemaphore, const vk::Semaphore& renderFinishedSemaphore,
	                     const vk::Queue& graphicsQueue, const vk::Queue& presentQueue,
	                     const vk::SwapchainKHR swapchain, const u32& imageIndex);
}
