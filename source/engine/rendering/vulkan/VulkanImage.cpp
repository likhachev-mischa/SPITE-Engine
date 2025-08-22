#include "VulkanImage.hpp"

#include "Base/Assert.hpp"

namespace spite
{
	VulkanImage::VulkanImage(VmaAllocator allocator, const vk::ImageCreateInfo& imageInfo,
	                         const VmaAllocationCreateInfo& allocInfo)
		: m_allocator(allocator)
	{
		auto result = vmaCreateImage(m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageInfo), &allocInfo,
		                             reinterpret_cast<VkImage*>(&m_image), &m_allocation, nullptr);
		SASSERTM(result == VK_SUCCESS, "Failed to create an image\n")
	}

	VulkanImage::VulkanImage(vk::Image externalImage)
		: m_image(externalImage), m_isExternal(true)
	{
	}

	VulkanImage::~VulkanImage()
	{
		if (!m_isExternal && m_allocator)
		{
			vmaDestroyImage(m_allocator, m_image, m_allocation);
		}
	}

	VulkanImage::VulkanImage(VulkanImage&& other) noexcept
		: m_allocator(other.m_allocator),
		  m_image(other.m_image),
		  m_allocation(other.m_allocation),
		  m_isExternal(other.m_isExternal)
	{
		other.m_allocator = nullptr;
		other.m_image = nullptr;
		other.m_allocation = nullptr;
		other.m_isExternal = false;
	}

	VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept
	{
		if (this != &other)
		{
			if (!m_isExternal && m_allocator)
			{
				vmaDestroyImage(m_allocator, m_image, m_allocation);
			}
			m_allocator = other.m_allocator;
			m_image = other.m_image;
			m_allocation = other.m_allocation;
			m_isExternal = other.m_isExternal;
			other.m_allocator = nullptr;
			other.m_image = nullptr;
			other.m_allocation = nullptr;
			other.m_isExternal = false;
		}
		return *this;
	}
}
