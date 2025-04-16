#include "VulkanRendering.hpp"

#include "Base/Assert.hpp"
#include "Base/Logging.hpp"

#include "Engine/Common.hpp"

namespace spite
{
	void beginSecondaryCommandBuffer(const vk::CommandBuffer& commandBuffer,
	                                 const vk::RenderPass& renderPass,
	                                 const vk::Framebuffer& framebuffer)
	{
		vk::Result result = commandBuffer.reset();
		SASSERT_VULKAN(result)

		vk::CommandBufferInheritanceInfo inheritanceInfo(renderPass, 0, framebuffer);
		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue,
		                                     &inheritanceInfo);

		result = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(result)
	}

	void recordSecondaryCommandBuffer(const vk::CommandBuffer& commandBuffer,
	                                  const vk::Pipeline& graphicsPipeline,
	                                  const vk::PipelineLayout& pipelineLayout,
	                                  const std::vector<vk::DescriptorSet>& descriptorSets,
	                                  const vk::Extent2D& swapchainExtent,
	                                  const vk::Buffer& indexBuffer,
	                                  const vk::Buffer& vertexBuffer,
	                                  const u32* dynamicOffsets,
	                                  const vk::DeviceSize& indicesOffset,
	                                  const u32 indicesCount)

	{
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
		vk::DeviceSize offset = 0;

		commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, &offset);
		commandBuffer.bindIndexBuffer(indexBuffer, indicesOffset, vk::IndexType::eUint32);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                                 pipelineLayout,
		                                 0,
		                                 descriptorSets.size(),
		                                 descriptorSets.data(),
		                                 0,
		                                 dynamicOffsets);


		vk::Rect2D renderArea({}, swapchainExtent);
		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(swapchainExtent.width),
		                      static_cast<float>(swapchainExtent.height),
		                      0.0f,
		                      1.0f);
		commandBuffer.setViewport(0, 1, &viewport);


		commandBuffer.setScissor(0, 1, &renderArea);

		commandBuffer.drawIndexed(indicesCount, 1, 0, 0, 0);
	}

	void endSecondaryCommandBuffer(const vk::CommandBuffer& commandBuffer)
	{
		vk::Result result = commandBuffer.end();
		SASSERT_VULKAN(result)
	}

	void recordPrimaryCommandBuffer(const vk::CommandBuffer& commandBuffer,
	                                const vk::Extent2D& swapchainExtent,
	                                const vk::RenderPass& renderPass,
	                                const vk::Framebuffer& framebuffer,
	                                const std::vector<vk::CommandBuffer>& secondaryCommandBuffer,
	                                const vk::Image image)
	{
		vk::Result result = commandBuffer.reset();
		SASSERT_VULKAN(result)

		vk::CommandBufferBeginInfo beginInfo;
		result = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(result)

		vk::Rect2D renderArea({}, swapchainExtent);
		vk::ClearValue clearColor({0.1f, 0.1f, 0.1f, 1.0f});
		vk::RenderPassBeginInfo renderPassInfo(renderPass, framebuffer, renderArea, 1, &clearColor);


		commandBuffer.beginRenderPass(renderPassInfo,
		                              vk::SubpassContents::eSecondaryCommandBuffers);

		commandBuffer.executeCommands(secondaryCommandBuffer);

		commandBuffer.endRenderPass();

		vk::ImageMemoryBarrier imageMemoryBarrier(vk::AccessFlagBits::eColorAttachmentWrite,
		                                          vk::AccessFlagBits::eMemoryRead,
		                                          vk::ImageLayout::eUndefined,
		                                          vk::ImageLayout::ePresentSrcKHR,
		                                          vk::QueueFamilyIgnored,
		                                          vk::QueueFamilyIgnored,
		                                          image,
		                                          vk::ImageSubresourceRange(
			                                          vk::ImageAspectFlagBits::eColor,
			                                          0,
			                                          1,
			                                          0,
			                                          1));

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                              vk::PipelineStageFlagBits::eBottomOfPipe,
		                              vk::DependencyFlags(),
		                              0,
		                              nullptr,
		                              0,
		                              nullptr,
		                              1,
		                              &imageMemoryBarrier);


		result = commandBuffer.end();
		SASSERT_VULKAN(result)
	}

	vk::Result waitForFrame(const vk::Device& device,
	                        const vk::SwapchainKHR swapchain,
	                        const vk::Fence& inFlightFence,
	                        const vk::Semaphore& imageAvaliableSemaphore,
	                        u32& imageIndex)
	{
		vk::Result result = device.waitForFences(1, &inFlightFence, vk::True, UINT64_MAX);
		SASSERT_VULKAN(result)

		result = device.resetFences({inFlightFence});
		SASSERT_VULKAN(result)

		result = device.acquireNextImageKHR(swapchain,
		                                    UINT64_MAX,
		                                    imageAvaliableSemaphore,
		                                    {},
		                                    &imageIndex);
		return result;
	}

	//command buffer has to be recorded before
	vk::Result drawFrame(const vk::CommandBuffer& commandBuffer,
	                     const vk::Fence& inFlightFence,
	                     const vk::Semaphore& imageAvaliableSemaphore,
	                     const vk::Semaphore& renderFinishedSemaphore,
	                     const vk::Queue& graphicsQueue,
	                     const vk::Queue& presentQueue,
	                     const vk::SwapchainKHR swapchain,
	                     const u32& imageIndex)
	{
		vk::Semaphore waitSemaphores[] = {imageAvaliableSemaphore};
		vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
		vk::Semaphore signalSemaphores[] = {renderFinishedSemaphore};


		vk::SubmitInfo submitInfo(1,
		                          waitSemaphores,
		                          waitStages,
		                          1,
		                          &commandBuffer,
		                          1,
		                          signalSemaphores);


		vk::Result result = graphicsQueue.submit({submitInfo}, inFlightFence);
		SASSERT_VULKAN(result)

		//	result = graphicsQueue.waitIdle();
		//SASSERT_VULKAN(result)

		vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, &swapchain, &imageIndex, &result);
		result = presentQueue.presentKHR(presentInfo);

		return result;
	}
}
