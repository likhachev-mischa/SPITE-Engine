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
	                                  const vk::Pipeline& pipeline,
	                                  const vk::PipelineLayout& pipelineLayout,
	                                  const std::vector<vk::DescriptorSet>& descriptorSets,
	                                  const vk::Extent2D& swapchainExtent,
	                                  const vk::Buffer& indexBuffer,
	                                  const vk::Buffer& vertexBuffer,
	                                  const u32* dynamicOffsets,
	                                  const vk::DeviceSize& indicesOffset,
	                                  const u32 indicesCount)

	{
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
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

	void endCommandBuffer(const vk::CommandBuffer& commandBuffer)
	{
		vk::Result result = commandBuffer.end();
		SASSERT_VULKAN(result)
	}

	void beginCommandBuffer(const vk::CommandBuffer commandBuffer)
	{
		vk::Result result = commandBuffer.reset();
		SASSERT_VULKAN(result)

		vk::CommandBufferBeginInfo beginInfo;
		result = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(result)
	}

	void recordDepthPass(const vk::CommandBuffer primaryCommandBuffer,
	                     const vk::CommandBuffer depthCommandBuffer,
	                     const vk::RenderPass depthRenderPass,
	                     const vk::Framebuffer depthFramebuffer,
	                     const vk::Extent2D swapchainExtent,
	                     const vk::Image depthImage)
	{
		vk::ClearValue depthClearValue;
		depthClearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
		vk::RenderPassBeginInfo depthPassInfo(depthRenderPass,
		                                      depthFramebuffer,
		                                      vk::Rect2D({}, swapchainExtent),
		                                      1,
		                                      &depthClearValue);

		primaryCommandBuffer.beginRenderPass(depthPassInfo,
		                                     vk::SubpassContents::eSecondaryCommandBuffers);
		primaryCommandBuffer.executeCommands(depthCommandBuffer);
		primaryCommandBuffer.endRenderPass();

		vk::ImageMemoryBarrier depthImageMemoryBarrier(
			vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eShaderRead,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eDepthStencilReadOnlyOptimal,
			vk::QueueFamilyIgnored,
			vk::QueueFamilyIgnored,
			depthImage,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));

		primaryCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eLateFragmentTests,
		                                     vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eFragmentShader,
		                                     vk::DependencyFlagBits::eByRegion,
		                                     0,
		                                     nullptr,
		                                     0,
		                                     nullptr,
		                                     1,
		                                     &depthImageMemoryBarrier);
	}

	void recordGeometryPass(const vk::CommandBuffer primaryCommandBuffer,
	                        const vk::CommandBuffer geometryCommandBuffer,
	                        const vk::RenderPass geometryRenderPass,
	                        const vk::Framebuffer geometryFramebuffer,
	                        const vk::Extent2D swapchainExtent,
	                        const std::array<vk::Image, 3>& gBufferImages)
	{
		std::array<vk::ClearValue, 3> clearValues;
		clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
		// Position (world space, might not need clearing or specific clear)
		clearValues[1].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
		// Normals (might not need clearing or specific clear)
		clearValues[2].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
		// Albedo/Color + Specular

		vk::RenderPassBeginInfo geometryPassBeginInfo(geometryRenderPass,
		                                              geometryFramebuffer,
		                                              vk::Rect2D({}, swapchainExtent),
		                                              static_cast<u32>(clearValues.size()),
		                                              clearValues.data());

		primaryCommandBuffer.beginRenderPass(geometryPassBeginInfo,
		                                     vk::SubpassContents::eSecondaryCommandBuffers);

		primaryCommandBuffer.executeCommands(geometryCommandBuffer);

		primaryCommandBuffer.endRenderPass();

		// Barriers for G-Buffer images to be readable by the lighting pass
		std::array<vk::ImageMemoryBarrier, 3> gBufferImageBarriers;

		for (u32 i = 0; i < gBufferImageBarriers.size(); ++i)
		{
			gBufferImageBarriers[i] = vk::ImageMemoryBarrier(
				vk::AccessFlagBits::eColorAttachmentWrite,
				vk::AccessFlagBits::eShaderRead,
				vk::ImageLayout::eShaderReadOnlyOptimal,
				// Or eShaderReadOnlyOptimal if render pass already transitioned it
				vk::ImageLayout::eShaderReadOnlyOptimal,
				vk::QueueFamilyIgnored,
				vk::QueueFamilyIgnored,
				gBufferImages[i],
				vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
		}

		primaryCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                     // Stage where G-Buffer writes finish
		                                     vk::PipelineStageFlagBits::eFragmentShader,
		                                     // Stage where lighting pass reads G-Buffer
		                                     vk::DependencyFlagBits::eByRegion,
		                                     0,
		                                     nullptr,
		                                     0,
		                                     nullptr,
		                                     static_cast<u32>(gBufferImageBarriers.size()),
		                                     gBufferImageBarriers.data());
	}

	void recordLightPass(const vk::CommandBuffer primaryCommandBuffer,
	                              const vk::CommandBuffer lightingCommandBuffer,
	                              const vk::RenderPass lightingRenderPass,
	                              const vk::Framebuffer lightingFramebuffer,
	                              const vk::Extent2D swapchainExtent,
	                              const vk::Image swapchainImage)
	{
		vk::ClearValue finalColorClearValue;
		finalColorClearValue.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}); // Clear for the final output

		vk::RenderPassBeginInfo lightingPassBeginInfo(lightingRenderPass,
			lightingFramebuffer,
			vk::Rect2D({}, swapchainExtent),
			1,
			&finalColorClearValue);

		primaryCommandBuffer.beginRenderPass(lightingPassBeginInfo,
			vk::SubpassContents::eSecondaryCommandBuffers);
		primaryCommandBuffer.executeCommands(lightingCommandBuffer);
		primaryCommandBuffer.endRenderPass();

		// Barrier for the swapchain image to be presentable
		// This might be handled by the lightingRenderPass's finalLayout if it's ePresentSrcKHR.
		// However, an explicit barrier ensures synchronization before presentation.
		vk::ImageMemoryBarrier swapchainImageBarrier(
			vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eMemoryRead, // For presentation engine
			vk::ImageLayout::ePresentSrcKHR, // Or eShaderReadOnlyOptimal if that was its last use before this pass, or eUndefined if first use
			vk::ImageLayout::ePresentSrcKHR,
			vk::QueueFamilyIgnored,
			vk::QueueFamilyIgnored,
			swapchainImage,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

		primaryCommandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput, // Stage where lighting pass write finishes
			vk::PipelineStageFlagBits::eBottomOfPipe,          // Stage for presentation
			vk::DependencyFlags(), // No specific dependency flags needed here usually
			0, nullptr,
			0, nullptr,
			1, &swapchainImageBarrier);
	}

	void recordPrimaryCommandBuffer(const vk::CommandBuffer& commandBuffer,
	                                const vk::Extent2D& swapchainExtent,
	                                const vk::RenderPass& geometryRenderPass,
	                                const vk::Framebuffer& geometryFramebuffer,
	                                const vk::RenderPass& depthRenderPass,
	                                const vk::Framebuffer& depthFramebuffer,
	                                const std::vector<vk::CommandBuffer>& secondaryCommandBuffer,
	                                const std::vector<vk::CommandBuffer>& depthCommandBuffer,
	                                const vk::Image image,
	                                const vk::Image depthImage)
	{
		vk::Result result = commandBuffer.reset();
		SASSERT_VULKAN(result)

		vk::CommandBufferBeginInfo beginInfo;
		result = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(result)

		//DEPTH
		vk::ClearValue depthClearValue;
		depthClearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
		vk::RenderPassBeginInfo depthPassInfo(depthRenderPass,
		                                      depthFramebuffer,
		                                      vk::Rect2D({}, swapchainExtent),
		                                      1,
		                                      &depthClearValue);

		commandBuffer.beginRenderPass(depthPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);
		commandBuffer.executeCommands(depthCommandBuffer);
		commandBuffer.endRenderPass();

		vk::ImageMemoryBarrier depthImageMemoryBarrier(
			vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			vk::AccessFlagBits::eDepthStencilAttachmentRead,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal,
			vk::QueueFamilyIgnored,
			vk::QueueFamilyIgnored,
			depthImage,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eLateFragmentTests,
		                              vk::PipelineStageFlagBits::eEarlyFragmentTests,
		                              vk::DependencyFlagBits::eByRegion,
		                              0,
		                              nullptr,
		                              0,
		                              nullptr,
		                              1,
		                              &depthImageMemoryBarrier);

		vk::ClearValue colorClearValue;
		colorClearValue.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

		vk::RenderPassBeginInfo colorPassBeginInfo(geometryRenderPass,
		                                           geometryFramebuffer,
		                                           vk::Rect2D({}, swapchainExtent),
		                                           1,
		                                           &colorClearValue);

		commandBuffer.beginRenderPass(colorPassBeginInfo,
		                              vk::SubpassContents::eSecondaryCommandBuffers);

		commandBuffer.executeCommands(secondaryCommandBuffer);

		commandBuffer.endRenderPass();

		vk::ImageMemoryBarrier imageMemoryBarrier(vk::AccessFlagBits::eColorAttachmentWrite,
		                                          vk::AccessFlagBits::eMemoryRead,
		                                          vk::ImageLayout::ePresentSrcKHR,
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
	vk::Result drawFrame(const std::vector<vk::CommandBuffer>& commandBuffers,
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
		                          commandBuffers.size(),
		                          commandBuffers.data(),
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
