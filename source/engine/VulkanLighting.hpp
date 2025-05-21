#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "base/Platform.hpp"
#include "base/VulkanUsage.hpp"

namespace spite
{
	struct alignas(16) PointLightData
	{
		glm::vec4 position; //w radius
		glm::vec4 color; //w intensity

		PointLightData() = default;

		PointLightData(const glm::vec3& position,
		               const glm::vec3& color,
		               const float intensity,
		               const float radius): position(position[0], position[1], position[2], radius),
		                                    color(color[0], color[1], color[2], intensity)
		{
		}
	};

	struct alignas(16) DirectionalLightData
	{
		glm::vec3 direction;
		glm::vec4 color;

		DirectionalLightData() = default;

		DirectionalLightData(const glm::vec3& direction,
		                     const glm::vec3& color,
		                     const float intensity) : direction(direction),
		                                              color(color[0], color[1], color[2], intensity)
		{
		}
	};

	struct alignas(16) SpotlightData
	{
		glm::vec4 position;
		glm::vec3 direction;
		glm::vec4 color;
		glm::vec2 cutoffs;

		SpotlightData() = default;

		SpotlightData(const glm::vec3 position,
		              const glm::vec3& direction,
		              const glm::vec3& color,
		              const float intensity,
		              const float radius,
		              const glm::vec2& cutoffs):
			position(position[0], position[1], position[2], radius), direction(direction),
			color(color[0], color[1], color[2], intensity), cutoffs(cutoffs)
		{
		}
	};

	struct CombinedLightData
	{
		PointLightData pointLight;
		DirectionalLightData directionalLight;
		SpotlightData spotlight;
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
