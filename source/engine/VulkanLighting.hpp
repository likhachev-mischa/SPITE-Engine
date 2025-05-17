#pragma once
#include <glm/vec4.hpp>

#include "base/Platform.hpp"
#include "base/VulkanUsage.hpp"

namespace spite
{
	struct alignas(16) PointLightData
	{
		glm::vec4 position; //w radius
		glm::vec4 color; //w intensity
	};

	vk::RenderPass createLightRenderPass(const vk::Device device,
	                                     const vk::Format swapchainImageFormat,
	                                     const vk::AllocationCallbacks& allocationCallbacks);

	vk::Pipeline createLightPipeline(const vk::Device& device,
	                                 const vk::PipelineLayout& pipelineLayout,
	                                 const vk::Extent2D& extent,
	                                 const vk::RenderPass& renderPass,
	                                 const std::vector<vk::PipelineShaderStageCreateInfo>&
	                                 shaderStages,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks);

	void setupLightDescriptors(const vk::Device device,
	                           const std::array<vk::ImageView, 3>& imageViews,
	                           const vk::Sampler sampler,
	                           const vk::DescriptorSet descriptorSet);

	void recordLightCommandBuffer(const vk::CommandBuffer& commandBuffer,
	                              const vk::RenderPass renderPass,
	                              const vk::Framebuffer framebuffer,
	                              const vk::Pipeline& pipeline,
	                              const vk::PipelineLayout& pipelineLayout,
	                              const std::vector<vk::DescriptorSet>& descriptorSets,
	                              const vk::Extent2D& swapchainExtent,
	                              const u32* dynamicOffsets);
}
