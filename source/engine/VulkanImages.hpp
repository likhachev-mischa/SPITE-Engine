#pragma once
#include "Base/VmaUsage.hpp"

#include "Engine/Common.hpp"

namespace spite
{
	struct Image
	{
		vk::ImageUsageFlags usageFlags;
		vk::Format format;
		vk::Image image;
		vma::Allocation allocation;

		Image() = default;

		Image(const vk::ImageUsageFlags& usageFlags,
		      const vk::Format& format,
		      const vk::Image& image,
		      const vma::Allocation& allocation);
	};

	Image createImage(const vk::Extent3D extent,
	                  const vk::Format format,
	                  const vk::ImageUsageFlags usageFlags,
	                  const vma::Allocator gpuAllocator,
	                  const vk::SharingMode sharingMode = vk::SharingMode::eExclusive,
	                  const u32* queueIndices = nullptr,
	                  const u32 queueIndicesCount = 0);

	vk::ImageView createImageView(const vk::Device device,
	                              const Image& image,
	                              const vk::ImageAspectFlags imageAspectFlags,
	                              const vk::AllocationCallbacks& allocationCallbacks);
}
