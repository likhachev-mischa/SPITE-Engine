#include "VulkanPipeline.hpp"
#include "base/Assert.hpp"

namespace spite
{
    VulkanPipeline::VulkanPipeline(vk::Device device, vk::PipelineCache cache, PipelineLayoutHandle layoutHandle, const vk::GraphicsPipelineCreateInfo& createInfo)
        : m_device(device), m_layoutHandle(layoutHandle)
    {
        m_layout = createInfo.layout;
        auto pipeline = m_device.createGraphicsPipeline(cache, createInfo);
        SASSERT_VULKAN(pipeline.result)
        m_pipeline = pipeline.value;
    }

    VulkanPipeline::~VulkanPipeline()
    {
        if (m_pipeline)
        {
            m_device.destroyPipeline(m_pipeline);
        }
    }

    VulkanPipeline::VulkanPipeline(VulkanPipeline&& other) noexcept
        : m_device(other.m_device),
          m_pipeline(other.m_pipeline),
          m_layout(other.m_layout),
		  m_layoutHandle(other.m_layoutHandle)
    {
        other.m_device = nullptr;
        other.m_pipeline = nullptr;
        other.m_layout = nullptr;
		other.m_layoutHandle = {};
    }

    VulkanPipeline& VulkanPipeline::operator=(VulkanPipeline&& other) noexcept
    {
        if (this != &other)
        {
            if (m_pipeline) { m_device.destroyPipeline(m_pipeline); }
            m_device = other.m_device;
            m_pipeline = other.m_pipeline;
            m_layout = other.m_layout;
			m_layoutHandle = other.m_layoutHandle;
            other.m_device = nullptr;
            other.m_pipeline = nullptr;
            other.m_layout = nullptr;
			other.m_layoutHandle = {};
        }
        return *this;
    }
}
