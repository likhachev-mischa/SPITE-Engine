#pragma once
#include "base/VulkanUsage.hpp"
#include "engine/rendering/IPipeline.hpp"
#include "engine/rendering/RenderResourceHandles.hpp"

namespace spite
{
	class VulkanPipeline : public IPipeline
	{
	public:
		VulkanPipeline() = default;
		VulkanPipeline(vk::Device device, vk::PipelineCache cache, PipelineLayoutHandle layoutHandle,
		               const vk::GraphicsPipelineCreateInfo& createInfo);
		~VulkanPipeline() override;

		VulkanPipeline(const VulkanPipeline&) = delete;
		VulkanPipeline& operator=(const VulkanPipeline&) = delete;
		VulkanPipeline(VulkanPipeline&& other) noexcept;
		VulkanPipeline& operator=(VulkanPipeline&& other) noexcept;

		vk::Pipeline get() const { return m_pipeline; }
		vk::PipelineLayout getLayout() const { return m_layout; }
		PipelineLayoutHandle getLayoutHandle() const { return m_layoutHandle; }

	private:
		vk::Device m_device = nullptr;
		vk::Pipeline m_pipeline = nullptr;
		vk::PipelineLayout m_layout = nullptr;
		PipelineLayoutHandle m_layoutHandle;
	};
}
