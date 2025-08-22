#pragma once
#include "base/VulkanUsage.hpp"
#include "engine/rendering/IResourceSetLayout.hpp"

namespace spite
{
    class VulkanDescriptorSetLayout : public IResourceSetLayout
    {
    public:
        VulkanDescriptorSetLayout() = default;
        VulkanDescriptorSetLayout(vk::Device device, const vk::DescriptorSetLayoutCreateInfo& createInfo);
        ~VulkanDescriptorSetLayout() override;

        VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
        VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;
        VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept;
        VulkanDescriptorSetLayout& operator=(VulkanDescriptorSetLayout&& other) noexcept;

        vk::DescriptorSetLayout get() const { return m_layout; }

    private:
        vk::Device m_device = nullptr;
        vk::DescriptorSetLayout m_layout = nullptr;
    };
}