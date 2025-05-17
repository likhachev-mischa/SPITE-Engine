#pragma once
#include "Base/Platform.hpp"
#include "Base/VulkanUsage.hpp"

namespace spite
{
	void beginSecondaryCommandBuffer(const vk::CommandBuffer& commandBuffer,
	                                 const vk::RenderPass& renderPass,
	                                 const vk::Framebuffer& framebuffer);

	void recordSecondaryCommandBuffer(const vk::CommandBuffer& commandBuffer,
	                                  const vk::Pipeline& pipeline,
	                                  const vk::PipelineLayout& pipelineLayout,
	                                  const std::vector<vk::DescriptorSet>& descriptorSets,
	                                  const vk::Extent2D& swapchainExtent,
	                                  const vk::Buffer& indexBuffer,
	                                  const vk::Buffer& vertexBuffer,
	                                  const u32* dynamicOffsets,
	                                  const vk::DeviceSize& indicesOffset,
	                                  const u32 indicesCount);

	void endCommandBuffer(const vk::CommandBuffer& commandBuffer);

	void beginCommandBuffer(const vk::CommandBuffer commandBuffer);

	void recordDepthPass(const vk::CommandBuffer primaryCommandBuffer,
		const vk::CommandBuffer depthCommandBuffer,
		const vk::RenderPass depthRenderPass,
		const vk::Framebuffer depthFramebuffer,
		const vk::Extent2D swapchainExtent,
		const vk::Image depthImage);

	void recordGeometryPass(const vk::CommandBuffer primaryCommandBuffer,
		const vk::CommandBuffer geometryCommandBuffer,
		const vk::RenderPass geometryRenderPass,
		const vk::Framebuffer geometryFramebuffer,
		const vk::Extent2D swapchainExtent,
		const std::array<vk::Image, 3>& gBufferImages);

	void recordLightPass(const vk::CommandBuffer primaryCommandBuffer,
		const vk::CommandBuffer lightingCommandBuffer,
		const vk::RenderPass lightingRenderPass,
		const vk::Framebuffer lightingFramebuffer,
		const vk::Extent2D swapchainExtent,
		const vk::Image swapchainImage);

	void recordPrimaryCommandBuffer(const vk::CommandBuffer& commandBuffer,
	                                const vk::Extent2D& swapchainExtent,
	                                const vk::RenderPass& geometryRenderPass,
	                                const vk::Framebuffer& geometryFramebuffer,
	                                const vk::RenderPass& depthRenderPass,
	                                const vk::Framebuffer& depthFramebuffer,
	                                const std::vector<vk::CommandBuffer>& secondaryCommandBuffer,
	                                const std::vector<vk::CommandBuffer>& depthCommandBuffer,
	                                const vk::Image image,const vk::Image depthImage);

	vk::Result waitForFrame(const vk::Device& device,
	                        const vk::SwapchainKHR swapchain,
	                        const vk::Fence& inFlightFence,
	                        const vk::Semaphore& imageAvaliableSemaphore,
	                        u32& imageIndex);

	vk::Result drawFrame(const std::vector<vk::CommandBuffer>& commandBuffers,
	                     const vk::Fence& inFlightFence,
	                     const vk::Semaphore& imageAvaliableSemaphore,
	                     const vk::Semaphore& renderFinishedSemaphore,
	                     const vk::Queue& graphicsQueue,
	                     const vk::Queue& presentQueue,
	                     const vk::SwapchainKHR swapchain,
	                     const u32& imageIndex);
}
