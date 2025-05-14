#pragma once
#include "Base/VmaUsage.hpp"

#include "Engine/Common.hpp"

namespace spite
{
	struct Image
	{
		vk::Image image;
		vma::Allocation allocation;
	};

	Image createDepthImage(const QueueFamilyIndices& queueFamilyIndices,
	                              const vk::Extent2D extent,
	                              const vma::Allocator gpuAllocator);

	vk::ImageView createDepthImageView(const vk::Device device,
	                                   const vk::Image image,
	                                   const vk::AllocationCallbacks& allocationCallbacks);
}
