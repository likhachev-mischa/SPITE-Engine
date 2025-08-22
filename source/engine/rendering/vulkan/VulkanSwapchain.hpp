#pragma once
#include "base/VulkanUsage.hpp"

namespace spite
{
	class VulkanSwapchain
	{
	public:
		VulkanSwapchain() = default;
		VulkanSwapchain(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface, int width,
		                int height);
		~VulkanSwapchain();

		void recreate(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface, int width,
		              int height);
		void destroy();
		void create(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface, int width,
		            int height);

		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
		VulkanSwapchain(VulkanSwapchain&& other) noexcept;
		VulkanSwapchain& operator=(VulkanSwapchain&& other) noexcept;

		vk::SwapchainKHR get() const { return m_swapchain; }
		vk::Format getImageFormat() const { return m_imageFormat; }
		vk::Extent2D getExtent() const { return m_extent; }
		const std::vector<vk::Image>& getImages() const { return m_images; }
		const std::vector<vk::ImageView>& getImageViews() const { return m_imageViews; }

	private:
		vk::Device m_device = nullptr;
		vk::SwapchainKHR m_swapchain = nullptr;
		vk::Format m_imageFormat;
		vk::Extent2D m_extent;
		std::vector<vk::Image> m_images;
		std::vector<vk::ImageView> m_imageViews;
	};
}
