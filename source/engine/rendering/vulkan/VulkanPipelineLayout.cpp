#include "VulkanPipelineLayout.hpp"

#include "base/Assert.hpp"

namespace spite
{
    VulkanPipelineLayout::VulkanPipelineLayout(vk::Device device, const vk::PipelineLayoutCreateInfo& createInfo)
        : m_device(device)
    {
        auto[result, layout] = m_device.createPipelineLayout(createInfo);
        SASSERT_VULKAN(result)
        m_pipelineLayout = layout;
    }

    VulkanPipelineLayout::~VulkanPipelineLayout()
    {
        if (m_pipelineLayout)
        {
            m_device.destroyPipelineLayout(m_pipelineLayout);
        }
    }

    VulkanPipelineLayout::VulkanPipelineLayout(VulkanPipelineLayout&& other) noexcept
        : m_device(other.m_device),
          m_pipelineLayout(other.m_pipelineLayout)
    {
        other.m_device = nullptr;
        other.m_pipelineLayout = nullptr;
    }

    VulkanPipelineLayout& VulkanPipelineLayout::operator=(VulkanPipelineLayout&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pipelineLayout) { m_device.destroyPipelineLayout(m_pipelineLayout); }
            m_device = other.m_device;
            m_pipelineLayout = other.m_pipelineLayout;
            other.m_device = nullptr;
            other.m_pipelineLayout = nullptr;
        }
        return *this;
    }
}
