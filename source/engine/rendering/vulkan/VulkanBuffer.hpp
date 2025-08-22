#pragma once
#include "base/VmaUsage.hpp"

#include "engine/rendering/IBuffer.hpp"

namespace spite
{
    class VulkanBuffer : public IBuffer
    {
    public:
        VulkanBuffer() = default;
        VulkanBuffer(vk::Device device, VmaAllocator allocator, const vk::BufferCreateInfo& bufferInfo, const VmaAllocationCreateInfo& allocInfo);
        ~VulkanBuffer() override;

        VulkanBuffer(const VulkanBuffer&) = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        VulkanBuffer(VulkanBuffer&& other) noexcept;
        VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

        vk::Buffer get() const { return m_buffer; }
        VmaAllocation getAllocation() const { return m_allocation; }
		vk::DeviceAddress getDeviceAddress() const { return m_deviceAddress; }

    private:
		vk::Device m_device = nullptr;
        VmaAllocator m_allocator = nullptr;
        vk::Buffer m_buffer = nullptr;
        VmaAllocation m_allocation = nullptr;
		vk::DeviceAddress m_deviceAddress = 0;
    };
}
