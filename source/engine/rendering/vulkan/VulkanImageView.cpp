#include "VulkanImageView.hpp"

#include "Base/Assert.hpp"

namespace spite
{
	VulkanImageView::VulkanImageView()
	{}

	VulkanImageView::VulkanImageView(vk::Device device, vk::Image image, vk::Format format,
	                                 vk::ImageAspectFlags aspectFlags)
		: m_device(device)
	{
		vk::ImageViewCreateInfo createInfo{};
		createInfo.image = image;
		createInfo.viewType = vk::ImageViewType::e2D;
		createInfo.format = format;
		createInfo.components.r = vk::ComponentSwizzle::eIdentity;
		createInfo.components.g = vk::ComponentSwizzle::eIdentity;
		createInfo.components.b = vk::ComponentSwizzle::eIdentity;
		createInfo.components.a = vk::ComponentSwizzle::eIdentity;
		createInfo.subresourceRange.aspectMask = aspectFlags;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		auto [result, imageView] = m_device.createImageView(createInfo);
		SASSERT_VULKAN(result)

		m_imageView = imageView;
	}

	VulkanImageView::VulkanImageView(vk::ImageView externalImageView)
		: m_imageView(externalImageView), m_isExternal(true)
	{
	}

	VulkanImageView::~VulkanImageView()
	{
		if (!m_isExternal && m_imageView)
		{
			m_device.destroyImageView(m_imageView);
		}
	}

	VulkanImageView::VulkanImageView(VulkanImageView&& other) noexcept
		: m_device(other.m_device),
		  m_imageView(other.m_imageView),
		  m_isExternal(other.m_isExternal)
	{
		other.m_device = nullptr;
		other.m_imageView = nullptr;
		other.m_isExternal = false;
	}

	VulkanImageView& VulkanImageView::operator=(VulkanImageView&& other) noexcept
	{
		if (this != &other)
		{
			if (!m_isExternal && m_imageView) { m_device.destroyImageView(m_imageView); }
			m_device = other.m_device;
			m_imageView = other.m_imageView;
			m_isExternal = other.m_isExternal;
			other.m_device = nullptr;
			other.m_imageView = nullptr;
			other.m_isExternal = false;
		}
		return *this;
	}
}
