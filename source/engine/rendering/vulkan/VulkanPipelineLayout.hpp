#pragma once
#include "base/VulkanUsage.hpp"
#include "engine/rendering/IPipelineLayout.hpp"

namespace spite
{
    class VulkanPipelineLayout : public IPipelineLayout
    {
    public:
        VulkanPipelineLayout() = default;
        VulkanPipelineLayout(vk::Device device, const vk::PipelineLayoutCreateInfo& createInfo);
        ~VulkanPipelineLayout() override;

        VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
        VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;
        VulkanPipelineLayout(VulkanPipelineLayout&& other) noexcept;
        VulkanPipelineLayout& operator=(VulkanPipelineLayout&& other) noexcept;

        vk::PipelineLayout get() const { return m_pipelineLayout; }

    private:
        vk::Device m_device = nullptr;
        vk::PipelineLayout m_pipelineLayout = nullptr;
    };
}
