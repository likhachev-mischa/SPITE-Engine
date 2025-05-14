#include "VulkanImages.hpp"

namespace spite
{
	Image createDepthImage(const QueueFamilyIndices& queueFamilyIndices,
	                       const vk::Extent2D extent,
	                       const vma::Allocator gpuAllocator)
	{
		vk::Extent3D depthExtent(extent.width, extent.height, 1); // Depth must be at least 1
		u32 indices[] = {
			queueFamilyIndices.presentFamily.value(), queueFamilyIndices.graphicsFamily.value(),
			queueFamilyIndices.transferFamily.value()
		};
		vk::ImageCreateInfo depthImageCreateInfo({},
		                                         vk::ImageType::e2D,
		                                         vk::Format::eD32Sfloat,
		                                         depthExtent,
		                                         1,
		                                         // mipLevels must be at least 1
		                                         1,
		                                         // arrayLayers must be at least 1
		                                         vk::SampleCountFlagBits::e1,
		                                         // Specify sample count
		                                         vk::ImageTiling::eOptimal,
		                                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
		                                         vk::SharingMode::eConcurrent,
		                                         // If using multiple queue families
		                                         3,
		                                         indices);

		auto [result, depthImage] = gpuAllocator.createImage(depthImageCreateInfo,
		                                                     {
			                                                     {}, {},
			                                                     vk::MemoryPropertyFlagBits::eDeviceLocal
		                                                     });
		SASSERT_VULKAN(result)

		Image image;
		image.image = depthImage.first;
		image.allocation = depthImage.second;
		return image;
	}

	vk::ImageView createDepthImageView(const vk::Device device,
	                                   const vk::Image image,
	                                   const vk::AllocationCallbacks& allocationCallbacks)
	{
		vk::ImageSubresourceRange subresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);

		auto [result, imageView] = device.createImageView({
			                                                  {}, image, vk::ImageViewType::e2D,
			                                                  vk::Format::eD32Sfloat, {},
			                                                  subresourceRange
		                                                  },
		                                                  allocationCallbacks);
		SASSERT_VULKAN(result)
		return imageView;
	}
}
