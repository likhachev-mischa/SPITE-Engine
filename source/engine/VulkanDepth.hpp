#pragma once
#include "Base/Platform.hpp"
#include "base/VulkanUsage.hpp"

namespace spite
{
	vk::RenderPass createDepthRenderPass(const vk::Device& device,
	                                     const vk::AllocationCallbacks* pAllocationCallbacks);
	
	vk::Pipeline createDepthPipeline(const vk::Device& device,
	                                 const vk::PipelineLayout& pipelineLayout,
	                                 const vk::Extent2D& swapchainExtent,
	                                 const vk::RenderPass& renderPass,
	                                 const std::vector<vk::PipelineShaderStageCreateInfo>&
	                                 shaderStages,
	                                 const vk::PipelineVertexInputStateCreateInfo& vertexInputInfo,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks);
}
