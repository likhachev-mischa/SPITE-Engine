#include "VulkanBuffer.hpp"

#include "Base/Assert.hpp"

namespace spite
{
	VulkanBuffer::VulkanBuffer(vk::Device device, VmaAllocator allocator, const vk::BufferCreateInfo& bufferInfo,
	                           const VmaAllocationCreateInfo& allocInfo)
		: m_device(device), m_allocator(allocator)
	{
		auto result = vmaCreateBuffer(m_allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferInfo), &allocInfo,
		                              reinterpret_cast<VkBuffer*>(&m_buffer), &m_allocation, nullptr);
		SASSERTM(result == VK_SUCCESS, "Failed to create a buffer\n")

		if (bufferInfo.usage & vk::BufferUsageFlagBits::eShaderDeviceAddress)
		{
			vk::BufferDeviceAddressInfo addressInfo{m_buffer};
			m_deviceAddress = m_device.getBufferAddress(&addressInfo);
		}
	}

	VulkanBuffer::~VulkanBuffer()
	{
		if (m_allocator)
		{
			vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
		}
	}

	VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
		: m_device(other.m_device),
		  m_allocator(other.m_allocator),
		  m_buffer(other.m_buffer),
		  m_allocation(other.m_allocation),
		  m_deviceAddress(other.m_deviceAddress)
	{
		other.m_device = nullptr;
		other.m_allocator = nullptr;
		other.m_buffer = nullptr;
		other.m_allocation = nullptr;
		other.m_deviceAddress = 0;
	}

	VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
	{
		if (this != &other)
		{
			vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
			m_device = other.m_device;
			m_allocator = other.m_allocator;
			m_buffer = other.m_buffer;
			m_allocation = other.m_allocation;
			m_deviceAddress = other.m_deviceAddress;
			other.m_device = nullptr;
			other.m_allocator = nullptr;
			other.m_buffer = nullptr;
			other.m_allocation = nullptr;
			other.m_deviceAddress = 0;
		}
		return *this;
	}
}
