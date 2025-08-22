#include "VulkanDescriptorSetLayout.hpp"

#include "Base/Assert.hpp"

namespace spite
{
	VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(vk::Device device,
	                                                     const vk::DescriptorSetLayoutCreateInfo& createInfo)
		: m_device(device)
	{
		auto [result, layout] = m_device.createDescriptorSetLayout(createInfo);
		SASSERT_VULKAN(result)
		m_layout = layout;
	}

	VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
	{
		if (m_layout)
		{
			m_device.destroyDescriptorSetLayout(m_layout);
		}
	}

	VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept
		: m_device(other.m_device),
		  m_layout(other.m_layout)
	{
		other.m_device = nullptr;
		other.m_layout = nullptr;
	}

	VulkanDescriptorSetLayout& VulkanDescriptorSetLayout::operator=(VulkanDescriptorSetLayout&& other) noexcept
	{
		if (this != &other)
		{
			if (m_layout) { m_device.destroyDescriptorSetLayout(m_layout); }
			m_device = other.m_device;
			m_layout = other.m_layout;
			other.m_device = nullptr;
			other.m_layout = nullptr;
		}
		return *this;
	}
}
