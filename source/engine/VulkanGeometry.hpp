#pragma once
#include "base/VulkanUsage.hpp"

namespace spite
{
	
	vk::RenderPass createGeometryRenderPass(const vk::Device& device,
	                                        const vk::AllocationCallbacks* pAllocationCallbacks);

	vk::Pipeline createGeometryPipeline(const vk::Device& device,
	                                    const vk::PipelineLayout& pipelineLayout,
	                                    const vk::Extent2D& swapchainExtent,
	                                    const vk::RenderPass& renderPass,
	                                    const std::vector<vk::PipelineShaderStageCreateInfo>&
	                                    shaderStages,
	                                    const vk::PipelineVertexInputStateCreateInfo&
	                                    vertexInputInfo,
	                                    const vk::AllocationCallbacks* pAllocationCallbacks);
}

