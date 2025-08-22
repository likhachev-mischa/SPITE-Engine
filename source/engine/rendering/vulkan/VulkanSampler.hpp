#pragma once
#include "base/VulkanUsage.hpp"

#include "engine/rendering/ISampler.hpp"

namespace spite
{
	class VulkanSampler : public ISampler
	{
	public:
		VulkanSampler();
		VulkanSampler(vk::Device device, const vk::SamplerCreateInfo& createInfo);
		~VulkanSampler() override;

		VulkanSampler(const VulkanSampler&) = delete;
		VulkanSampler& operator=(const VulkanSampler&) = delete;
		VulkanSampler(VulkanSampler&& other) noexcept;
		VulkanSampler& operator=(VulkanSampler&& other) noexcept;

		vk::Sampler get() const { return m_sampler; }

	private:
		vk::Device m_device = nullptr;
		vk::Sampler m_sampler = nullptr;
	};
}
