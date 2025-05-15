#include "VulkanImages.hpp"

namespace spite
{
	Image::Image(const vk::ImageUsageFlags& usageFlags,
	             const vk::Format& format,
	             const vk::Image& image,
	             const vma::Allocation& allocation): usageFlags(usageFlags), format(format),
	                                                 image(image), allocation(allocation)
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

		Image image(usageFlags, format, imagePair.first, imagePair.second);
		return image;
	}

	vk::ImageView createImageView(const vk::Device device,
	                              const Image& image,const vk::ImageAspectFlags imageAspectFlags,
	                              const vk::AllocationCallbacks& allocationCallbacks)
	{
		vk::ImageSubresourceRange subresourceRange(imageAspectFlags, 0, 1, 0, 1);

		auto [result, imageView] = device.createImageView({
			                                                  {}, image.image, vk::ImageViewType::e2D,
			                                                  image.format, {},
			                                                  subresourceRange
		                                                  },
		                                                  allocationCallbacks);
		SASSERT_VULKAN(result)
		return imageView;
	}

	vk::Sampler createSampler(const vk::Device device,
		const vk::AllocationCallbacks& allocationCallbacks)
	{
		vk::SamplerCreateInfo samplerCreateInfo({},
		                                        vk::Filter::eLinear,
		                                        vk::Filter::eLinear,
		                                        vk::SamplerMipmapMode::eLinear,
		                                        vk::SamplerAddressMode::eClampToEdge,
		                                        vk::SamplerAddressMode::eClampToEdge,
		                                        vk::SamplerAddressMode::eClampToEdge,
		                                        {},
		                                        vk::False,
		                                        {},
		                                        vk::False,
		                                        {},
		                                        {},
		                                        {},
		                                        vk::BorderColor::eFloatOpaqueWhite);
		auto [result, sampler] = device.createSampler(samplerCreateInfo, &allocationCallbacks);
		SASSERT_VULKAN(result);
		return sampler;
	}

	std::vector<vk::Framebuffer> createFramebuffers(const sizet swapchainImagesCount,
	                                                const vk::Device& device,
	                                                const std::vector<vk::ImageView>& imageViews,
	                                                const vk::Extent2D& swapchainExtent,
	                                                const vk::RenderPass& renderPass,
	                                                const vk::AllocationCallbacks* pAllocationCallbacks)
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
		const std::vector<vk::ImageView>& swapchainImageViews,
		const std::vector<vk::ImageView>& otherAttachments,
		const vk::Extent2D& swapchainExtent,
		const vk::RenderPass& renderPass,
		const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		std::vector<vk::Framebuffer> swapchainFramebuffers;
		swapchainFramebuffers.resize(swapchainImageViews.size());

		std::vector<vk::ImageView> attachments;
		u32 attachmentsSize = static_cast<u32>(otherAttachments.size()) + 1;
		attachments.reserve(attachmentsSize);

		attachments.push_back(swapchainImageViews[0]);
		for (const auto & attachment: otherAttachments)
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
