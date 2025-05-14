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
}
