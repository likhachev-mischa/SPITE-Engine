#pragma once
#include "base/VmaUsage.hpp"

namespace spite
{
    class VulkanImage
    {
    public:
        VulkanImage() = default;
        VulkanImage(VmaAllocator allocator, const vk::ImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocInfo);
        VulkanImage(vk::Image externalImage);
        ~VulkanImage();

        VulkanImage(const VulkanImage&) = delete;
        VulkanImage& operator=(const VulkanImage&) = delete;

        VulkanImage(VulkanImage&& other) noexcept;
        VulkanImage& operator=(VulkanImage&& other) noexcept;

        vk::Image get() const { return m_image; }
        VmaAllocation getAllocation() const { return m_allocation; }

    private:
        VmaAllocator m_allocator = nullptr;
        vk::Image m_image = nullptr;
        VmaAllocation m_allocation = nullptr;
        bool m_isExternal = false;
    };
}
