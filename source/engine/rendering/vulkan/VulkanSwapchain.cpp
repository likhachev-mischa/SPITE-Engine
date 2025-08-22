#include "VulkanSwapchain.hpp"
#include "base/Assert.hpp"

namespace spite
{
	VulkanSwapchain::VulkanSwapchain(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface,
	                                 int width, int height)
	{
		create(physicalDevice, device, surface, width, height);
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		destroy();
	}

	void VulkanSwapchain::recreate(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface,
	                               int width, int height)
	{
		destroy();
		create(physicalDevice, device, surface, width, height);
	}

	VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other) noexcept
		: m_device(other.m_device),
		  m_swapchain(other.m_swapchain),
		  m_imageFormat(other.m_imageFormat),
		  m_extent(other.m_extent),
		  m_images(std::move(other.m_images)),
		  m_imageViews(std::move(other.m_imageViews))
	{
		other.m_device = nullptr;
		other.m_swapchain = nullptr;
	}

	VulkanSwapchain& VulkanSwapchain::operator=(VulkanSwapchain&& other) noexcept
	{
		if (this != &other)
		{
			destroy();
			m_device = other.m_device;
			m_swapchain = other.m_swapchain;
			m_imageFormat = other.m_imageFormat;
			m_extent = other.m_extent;
			m_images = std::move(other.m_images);
			m_imageViews = std::move(other.m_imageViews);
			other.m_device = nullptr;
			other.m_swapchain = nullptr;
		}
		return *this;
	}

	void VulkanSwapchain::destroy()
	{
		if (!m_swapchain) return;

		for (auto imageView : m_imageViews)
		{
			if (imageView)
			{
				m_device.destroyImageView(imageView);
			}
		}
		m_imageViews.clear();

		m_device.destroySwapchainKHR(m_swapchain);
		m_swapchain = nullptr;
	}

	void VulkanSwapchain::create(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface,
	                             int width, int height)
	{
		m_device = device;

		// Query swapchain support details
		auto [res1,surfaceCapabilities] = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		SASSERT_VULKAN(res1)
		auto [res2,surfaceFormats] = physicalDevice.getSurfaceFormatsKHR(surface);
		SASSERT_VULKAN(res2)
		auto [res3,presentModes] = physicalDevice.getSurfacePresentModesKHR(surface);
		SASSERT_VULKAN(res2)

		// Choose swap surface format
		vk::SurfaceFormatKHR surfaceFormat = surfaceFormats[0];
		for (const auto& availableFormat : surfaceFormats)
		{
			if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace ==
				vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				surfaceFormat = availableFormat;
				break;
			}
		}
		m_imageFormat = surfaceFormat.format;

		// Choose swap present mode
		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
		for (const auto& availablePresentMode : presentModes)
		{
			if (availablePresentMode == vk::PresentModeKHR::eMailbox)
			{
				presentMode = availablePresentMode;
				break;
			}
		}

		// Choose swap extent
		if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
		{
			m_extent = surfaceCapabilities.currentExtent;
		}
		else
		{
			m_extent.width = static_cast<u32>(width);
			m_extent.height = static_cast<u32>(height);

			m_extent.width = std::clamp(m_extent.width, surfaceCapabilities.minImageExtent.width,
			                            surfaceCapabilities.maxImageExtent.width);
			m_extent.height = std::clamp(m_extent.height, surfaceCapabilities.minImageExtent.height,
			                             surfaceCapabilities.maxImageExtent.height);
		}

		u32 imageCount = surfaceCapabilities.minImageCount + 1;
		if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
		{
			imageCount = surfaceCapabilities.maxImageCount;
		}

		// Create swapchain
		vk::SwapchainCreateInfoKHR createInfo{};
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = m_imageFormat;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = m_extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

		// TODO: Handle different queue families
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.preTransform = surfaceCapabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		auto swapchain = m_device.createSwapchainKHR(createInfo);
		SASSERT_VULKAN(swapchain.result)
		m_swapchain = swapchain.value;

		// Get swapchain images
		auto images = m_device.getSwapchainImagesKHR(m_swapchain);
		SASSERT_VULKAN(images.result)
		m_images = images.value;

		// Create image views
		m_imageViews.resize(m_images.size());
		for (size_t i = 0; i < m_images.size(); i++)
		{
			vk::ImageViewCreateInfo viewInfo{};
			viewInfo.image = m_images[i];
			viewInfo.viewType = vk::ImageViewType::e2D;
			viewInfo.format = m_imageFormat;
			viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			auto imgView = m_device.createImageView(viewInfo);
			SASSERT_VULKAN(imgView.result)
			m_imageViews[i] = imgView.value;
		}
	}
}
