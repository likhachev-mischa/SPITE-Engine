#pragma once
#include "Base/Platform.hpp"
#include "Base/VulkanUsage.hpp"

namespace spite
{
	void beginSecondaryCommandBuffer(const vk::CommandBuffer& commandBuffer, const vk::RenderPass& renderPass,
	                                 const vk::Framebuffer& framebuffer);

	void recordSecondaryCommandBuffer(const vk::CommandBuffer& commandBuffer, const vk::Pipeline& graphicsPipeline,
	                                  const vk::PipelineLayout& pipelineLayout, const vk::DescriptorSet& descriptorSet,
	                                  const vk::Extent2D& swapchainExtent,
	                                  const vk::Buffer& buffer,const u32 dynamicOffset, const vk::DeviceSize& indicesOffset,
	                                  const u32 indicesCount);

	void endSecondaryCommandBuffer(const vk::CommandBuffer& commandBuffer);

	void recordPrimaryCommandBuffer(const vk::CommandBuffer& commandBuffer, const vk::Extent2D& swapchainExtent,
	                                const vk::RenderPass& renderPass, const vk::Framebuffer& framebuffer,
	                                const vk::CommandBuffer& secondaryCommandBuffer);

	vk::Result waitForFrame(const vk::Device& device, const vk::SwapchainKHR swapchain, const vk::Fence& inFlightFence,
	                        const vk::Semaphore& imageAvaliableSemaphore,
	                        u32& imageIndex);

	vk::Result drawFrame(const vk::CommandBuffer& commandBuffer, const vk::Fence& inFlightFence,
	                     const vk::Semaphore& imageAvaliableSemaphore, const vk::Semaphore& renderFinishedSemaphore,
	                     const vk::Queue& graphicsQueue, const vk::Queue& presentQueue,
	                     const vk::SwapchainKHR swapchain, const u32& imageIndex);
}
