#include "VulkanImages.hpp"
#include "base/File.hpp"

#include "engine/VulkanBuffer.hpp"
#include "engine/VulkanRendering.hpp"

namespace spite
{
	Image::Image(const vk::ImageUsageFlags& usageFlags,
	             const vk::Format& format,
	             const vk::Image& image,
	             const vk::Extent3D& size,
	             const vma::Allocation& allocation): usageFlags(usageFlags), format(format),
	                                                 image(image), size(size),
	                                                 allocation(allocation)
	{
	}

	Image createImage(const vk::Extent3D extent,
	                  const vk::Format format,
	                  const vk::ImageUsageFlags usageFlags,
	                  const vma::Allocator gpuAllocator,
	                  const vk::SharingMode sharingMode,
	                  const u32* queueIndices,
	                  const u32 queueIndicesCount)
	{
		vk::ImageCreateInfo imageCreateInfo({},
		                                    vk::ImageType::e2D,
		                                    format,
		                                    extent,
		                                    1,
		                                    1,
		                                    vk::SampleCountFlagBits::e1,
		                                    vk::ImageTiling::eOptimal,
		                                    usageFlags,
		                                    sharingMode,
		                                    queueIndicesCount,
		                                    queueIndices);

		auto [result, imagePair] = gpuAllocator.createImage(imageCreateInfo,
		                                                    {
			                                                    {}, {},
			                                                    vk::MemoryPropertyFlagBits::eDeviceLocal
		                                                    });

		SASSERT_VULKAN(result)

		Image image(usageFlags, format, imagePair.first, extent, imagePair.second);
		return image;
	}

	static void recordTransitionImageLayout(vk::CommandBuffer commandBuffer,
	                                        vk::Image image,
	                                        vk::ImageLayout oldLayout,
	                                        vk::ImageLayout newLayout,
	                                        vk::ImageAspectFlags aspectMask,
	                                        vk::PipelineStageFlags srcStageMask,
	                                        vk::PipelineStageFlags dstStageMask,
	                                        vk::AccessFlags srcAccessMask,
	                                        vk::AccessFlags dstAccessMask,
	                                        uint32_t mipLevels = 1,
	                                        uint32_t baseMipLevel = 0,
	                                        uint32_t layerCount = 1,
	                                        uint32_t baseArrayLayer = 0)
	{
		vk::ImageSubresourceRange subresourceRange(aspectMask,
		                                           baseMipLevel,
		                                           mipLevels,
		                                           baseArrayLayer,
		                                           layerCount);

		vk::ImageMemoryBarrier barrier(srcAccessMask,
		                               dstAccessMask,
		                               oldLayout,
		                               newLayout,
		                               vk::QueueFamilyIgnored,
		                               // srcQueueFamilyIndex
		                               vk::QueueFamilyIgnored,
		                               // dstQueueFamilyIndex
		                               image,
		                               subresourceRange);

		commandBuffer.pipelineBarrier(srcStageMask,
		                              dstStageMask,
		                              {},
		                              0,
		                              nullptr,
		                              0,
		                              nullptr,
		                              1,
		                              &barrier);
	}

	void copyBufferToImage(vk::Device device,
	                       vk::CommandPool transferCommandPool,
	                       vk::Queue transferQueue,
	                       vk::CommandPool graphicsCommandPool,
	                       vk::Queue graphicsQueue,
	                       vk::Buffer buffer,
	                       const Image& image,
	                       vk::ImageLayout initialLayout,
	                       vk::ImageLayout finalLayout,
	                       vk::ImageAspectFlags aspectFlags,
	                       uint32_t mipLevels = 1,
	                       uint32_t layerCount = 1)
	{
		vk::CommandBufferAllocateInfo allocInfo(transferCommandPool,
		                                        vk::CommandBufferLevel::ePrimary,
		                                        1);
		auto [result, transferCbs] = device.allocateCommandBuffers(allocInfo);
		SASSERT_VULKAN(result)

		vk::CommandBuffer transferCb = transferCbs[0];
		spite::beginCommandBuffer(transferCb);

		// Transition image to eTransferDstOptimal
		recordTransitionImageLayout(transferCb,
		                            image.image,
		                            initialLayout,
		                            vk::ImageLayout::eTransferDstOptimal,
		                            aspectFlags,
		                            vk::PipelineStageFlagBits::eTopOfPipe,
		                            vk::PipelineStageFlagBits::eTransfer,
		                            initialLayout == vk::ImageLayout::eUndefined
			                            ? vk::AccessFlagBits::eNone
			                            : vk::AccessFlagBits::eMemoryRead,
		                            // Adjust srcAccessMask if initialLayout could have pending writes
		                            vk::AccessFlagBits::eTransferWrite,
		                            mipLevels,
		                            0,
		                            layerCount,
		                            0);

		vk::BufferImageCopy region(0,
		                           // bufferOffset
		                           0,
		                           // bufferRowLength (0 means tightly packed)
		                           0,
		                           // bufferImageHeight (0 means tightly packed)
		                           vk::ImageSubresourceLayers(aspectFlags, 0, 0, layerCount),
		                           // imageSubresource (mipLevel 0, baseArrayLayer 0, layerCount)
		                           vk::Offset3D(0, 0, 0),
		                           // imageOffset
		                           image.size); // imageExtent

		transferCb.copyBufferToImage(buffer,
		                             image.image,
		                             vk::ImageLayout::eTransferDstOptimal,
		                             1,
		                             &region);

		spite::endCommandBuffer(transferCb);
		vk::SubmitInfo submitInfo({}, {}, {}, 1, &transferCb);
		result = transferQueue.submit(1, &submitInfo, nullptr);
		SASSERT_VULKAN(result)

		result = transferQueue.waitIdle();
		SASSERT_VULKAN(result)

		device.freeCommandBuffers(transferCommandPool,transferCbs);

		vk::CommandBufferAllocateInfo graphicsAllocInfo(graphicsCommandPool,
		                                                vk::CommandBufferLevel::ePrimary,
		                                                1);
		auto [result2, graphicsCbs] = device.allocateCommandBuffers(graphicsAllocInfo);
		SASSERT_VULKAN(result2)

		vk::CommandBuffer graphicsCb = graphicsCbs[0];
		spite::beginCommandBuffer(graphicsCb);

		recordTransitionImageLayout(graphicsCb,
		                            image.image,
		                            vk::ImageLayout::eTransferDstOptimal,
		                            finalLayout,
		                            aspectFlags,
		                            vk::PipelineStageFlagBits::eTransfer,
		                            vk::PipelineStageFlagBits::eFragmentShader,
		                            // Assuming shader read for textures
		                            vk::AccessFlagBits::eTransferWrite,
		                            vk::AccessFlagBits::eShaderRead,
		                            // Assuming shader read
		                            mipLevels,
		                            0,
		                            layerCount,
		                            0);

		spite::endCommandBuffer(graphicsCb);
		vk::SubmitInfo submitInfo2({}, {}, {}, 1, &graphicsCb);
		result = graphicsQueue.submit(1, &submitInfo2, nullptr);
		SASSERT_VULKAN(result)

		result = graphicsQueue.waitIdle();
		SASSERT_VULKAN(result)

		device.freeCommandBuffers(graphicsCommandPool,graphicsCbs);


		// Transition image to finalLayout
		//recordTransitionImageLayout(transferCb,
		//                            image.image,
		//                            vk::ImageLayout::eTransferDstOptimal,
		//                            finalLayout,
		//                            aspectFlags,
		//                            vk::PipelineStageFlagBits::eTransfer,
		//                            vk::PipelineStageFlagBits::eFragmentShader,
		//                            // Assuming shader read for textures
		//                            vk::AccessFlagBits::eTransferWrite,
		//                            vk::AccessFlagBits::eShaderRead,
		//                            // Assuming shader read
		//                            mipLevels,
		//                            0,
		//                            layerCount,
		//                            0);

		//spite::endCommandBuffer(transferCb);

		//vk::SubmitInfo submitInfo({}, {}, {}, 1, &transferCb);
		//result = transferQueue.submit(1, &submitInfo, nullptr);
		//SASSERT_VULKAN(result)

		//result = transferQueue.waitIdle();
		//SASSERT_VULKAN(result)

		//device.freeCommandBuffers(transferCommandPool, 1, &transferCb);
	}

	Image createTexture(const cstring path,
	                    const QueueFamilyIndices& queueFamilyIndices,
	                    const vma::Allocator& gpuAllocator,
	                    const vk::Device device,
	                    const vk::Queue transferQueue,
	                    const vk::CommandPool transferCommandPool,
	                    const vk::Queue graphicsQueue,
	                    const vk::CommandPool graphicsCommandPool)
	{
		int width, height,channels;
		u8* pixels = loadTexture(path, width, height, channels);
		channels = 4;

		vk::DeviceSize imageSize = width * height * channels;

		BufferWrapper stagingBuffer(imageSize,
		                            vk::BufferUsageFlagBits::eTransferSrc,
		                            vk::MemoryPropertyFlagBits::eHostVisible |
		                            vk::MemoryPropertyFlagBits::eHostCoherent,
		                            vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
		                            queueFamilyIndices,
		                            gpuAllocator);

		stagingBuffer.copyMemory(pixels, imageSize, 0);

		freeTexture(pixels);

		std::vector<u32> queueIndices = {
			queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.transferFamily.value()
		};
		Image image = createImage({static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
		                          vk::Format::eR8G8B8A8Srgb,
		                          vk::ImageUsageFlagBits::eTransferDst |
		                          vk::ImageUsageFlagBits::eSampled,
		                          gpuAllocator,
		                          vk::SharingMode::eConcurrent,
		                          queueIndices.data(),
		                          static_cast<u32>(queueIndices.size()));

		copyBufferToImage(device,
		                  transferCommandPool,
		                  transferQueue,
		                  graphicsCommandPool,
		                  graphicsQueue,
		                  stagingBuffer.buffer,
		                  image,
		                  vk::ImageLayout::eUndefined,
		                  vk::ImageLayout::eShaderReadOnlyOptimal,
		                  vk::ImageAspectFlagBits::eColor);
		return image;
	}

	vk::ImageView createImageView(const vk::Device device,
	                              const Image& image,
	                              const vk::ImageAspectFlags imageAspectFlags,
	                              const vk::AllocationCallbacks& allocationCallbacks)
	{
		vk::ImageSubresourceRange subresourceRange(imageAspectFlags, 0, 1, 0, 1);

		auto [result, imageView] = device.createImageView({
			                                                  {}, image.image,
			                                                  vk::ImageViewType::e2D, image.format,
			                                                  {}, subresourceRange
		                                                  },
		                                                  allocationCallbacks);
		SASSERT_VULKAN(result)
		return imageView;
	}

	vk::Sampler createSampler(const vk::Device device,
	                          const vk::AllocationCallbacks& allocationCallbacks)
	{
		vk::SamplerCreateInfo samplerCreateInfo({},
		                                        vk::Filter::eNearest,
		                                        vk::Filter::eNearest,
		                                        vk::SamplerMipmapMode::eNearest,
		                                        vk::SamplerAddressMode::eRepeat,
		                                        vk::SamplerAddressMode::eRepeat,
		                                        vk::SamplerAddressMode::eRepeat,
		                                        {},
		                                        vk::False,
		                                        {},
		                                        vk::False,
		                                        {},
		                                        {},
		                                        {},
		                                        vk::BorderColor::eFloatOpaqueBlack);
		auto [result, sampler] = device.createSampler(samplerCreateInfo, &allocationCallbacks);
		SASSERT_VULKAN(result);
		return sampler;
	}

	std::vector<vk::Framebuffer> createFramebuffers(const sizet swapchainImagesCount,
	                                                const vk::Device& device,
	                                                const std::vector<vk::ImageView>& imageViews,
	                                                const vk::Extent2D& swapchainExtent,
	                                                const vk::RenderPass& renderPass,
	                                                const vk::AllocationCallbacks*
	                                                pAllocationCallbacks)
	{
		std::vector<vk::Framebuffer> framebuffers;
		framebuffers.resize(swapchainImagesCount);

		for (size_t i = 0; i < swapchainImagesCount; ++i)
		{
			vk::FramebufferCreateInfo framebufferInfo({},
			                                          renderPass,
			                                          imageViews.size(),
			                                          imageViews.data(),
			                                          swapchainExtent.width,
			                                          swapchainExtent.height,
			                                          1);

			vk::Result result;
			std::tie(result, framebuffers[i]) = device.createFramebuffer(
				framebufferInfo,
				pAllocationCallbacks);
			SASSERT_VULKAN(result)
		}

		return framebuffers;
	}

	std::vector<vk::Framebuffer> createSwapchainFramebuffers(const vk::Device& device,
	                                                         const std::vector<vk::ImageView>&
	                                                         swapchainImageViews,
	                                                         const std::vector<vk::ImageView>&
	                                                         otherAttachments,
	                                                         const vk::Extent2D& swapchainExtent,
	                                                         const vk::RenderPass& renderPass,
	                                                         const vk::AllocationCallbacks*
	                                                         pAllocationCallbacks)
	{
		std::vector<vk::Framebuffer> swapchainFramebuffers;
		swapchainFramebuffers.resize(swapchainImageViews.size());

		std::vector<vk::ImageView> attachments;
		u32 attachmentsSize = static_cast<u32>(otherAttachments.size()) + 1;
		attachments.reserve(attachmentsSize);

		attachments.push_back(swapchainImageViews[0]);
		for (const auto& attachment : otherAttachments)
		{
			attachments.push_back(attachment);
		}


		for (size_t i = 0; i < swapchainImageViews.size(); ++i)
		{
			attachments[0] = swapchainImageViews[i];

			vk::FramebufferCreateInfo framebufferInfo({},
			                                          renderPass,
			                                          attachmentsSize,
			                                          attachments.data(),
			                                          swapchainExtent.width,
			                                          swapchainExtent.height,
			                                          1);

			vk::Result result;
			std::tie(result, swapchainFramebuffers[i]) = device.createFramebuffer(
				framebufferInfo,
				pAllocationCallbacks);
			SASSERT_VULKAN(result)
		}

		return swapchainFramebuffers;
	}
}
