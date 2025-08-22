#include "VulkanSampler.hpp"

#include "base/Assert.hpp"

namespace spite
{
	VulkanSampler::VulkanSampler()
	{
	}

	VulkanSampler::VulkanSampler(vk::Device device, const vk::SamplerCreateInfo& createInfo)
		: m_device(device)
	{
		auto sampler = m_device.createSampler(createInfo);
		SASSERT_VULKAN(sampler.result)
		m_sampler = sampler.value;
	}

	VulkanSampler::~VulkanSampler()
	{
		if (m_sampler)
		{
			m_device.destroySampler(m_sampler);
		}
	}

	VulkanSampler::VulkanSampler(VulkanSampler&& other) noexcept
		: m_device(other.m_device),
		  m_sampler(other.m_sampler)
	{
		other.m_device = nullptr;
		other.m_sampler = nullptr;
	}

	VulkanSampler& VulkanSampler::operator=(VulkanSampler&& other) noexcept
	{
		if (this != &other)
		{
			if (m_sampler) { m_device.destroySampler(m_sampler); }
			m_device = other.m_device;
			m_sampler = other.m_sampler;
			other.m_device = nullptr;
			other.m_sampler = nullptr;
		}
		return *this;
	}
}
