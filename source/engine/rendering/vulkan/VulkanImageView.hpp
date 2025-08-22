#pragma once
#include "base/VulkanUsage.hpp"

#include "engine/rendering/IImageView.hpp"

namespace spite
{
	class VulkanImageView : public IImageView
	{
	public:
		VulkanImageView();
		VulkanImageView(vk::Device device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
		VulkanImageView(vk::ImageView externalImageView);
		~VulkanImageView() override;

		VulkanImageView(const VulkanImageView&) = delete;
		VulkanImageView& operator=(const VulkanImageView&) = delete;
		VulkanImageView(VulkanImageView&& other) noexcept;
		VulkanImageView& operator=(VulkanImageView&& other) noexcept;

		vk::ImageView get() const { return m_imageView; }

	private:
		vk::Device m_device = nullptr;
		vk::ImageView m_imageView = nullptr;
		bool m_isExternal = false;
	};
}
