#include "VulkanDepth.hpp"

#include <EASTL/array.h>

#include "Engine/Common.hpp"

namespace spite
{
	vk::RenderPass createDepthRenderPass(const vk::Device& device,
	                                     const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::AttachmentDescription depthAttachment({},
		                                          vk::Format::eD32Sfloat,
		                                          vk::SampleCountFlagBits::e1,
		                                          vk::AttachmentLoadOp::eClear,
		                                          vk::AttachmentStoreOp::eDontCare,
		                                          vk::AttachmentLoadOp::eDontCare,
		                                          vk::AttachmentStoreOp::eStore,
		                                          vk::ImageLayout::eUndefined,
		                                          vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::AttachmentReference depthAttachmentRef(0,
		                                           vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::SubpassDescription subpass({},
		                               vk::PipelineBindPoint::eGraphics,
		                               {},
		                               {},
		                               {},
		                               {},
		                               nullptr,
		                               &depthAttachmentRef);

		vk::SubpassDependency dependency(vk::SubpassExternal,
		                                 0,
		                                 vk::PipelineStageFlagBits::eColorAttachmentOutput |
		                                 vk::PipelineStageFlagBits::eEarlyFragmentTests,
		                                 vk::PipelineStageFlagBits::eColorAttachmentOutput |
		                                 vk::PipelineStageFlagBits::eEarlyFragmentTests,
		                                 {},
		                                 vk::AccessFlagBits::eColorAttachmentWrite |
		                                 vk::AccessFlagBits::eDepthStencilAttachmentWrite);

		std::array<vk::AttachmentDescription, 1> attachments = {depthAttachment};
		vk::RenderPassCreateInfo renderPassInfo({},
		                                        static_cast<uint32_t>(attachments.size()),
		                                        attachments.data(),
		                                        1,
		                                        &subpass,
		                                        1,
		                                        &dependency);


		auto [result, renderPass] = device.createRenderPass(renderPassInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return renderPass;
	}

	vk::Pipeline createDepthPipeline(const vk::Device& device,
	                                 const vk::PipelineLayout& pipelineLayout,
	                                 const vk::Extent2D& swapchainExtent,
	                                 const vk::RenderPass& renderPass,
	                                 const std::vector<vk::PipelineShaderStageCreateInfo>&
	                                 shaderStages,
	                                 const vk::PipelineVertexInputStateCreateInfo& vertexInputInfo,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		eastl::array dynamicStates = {
			vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eLineWidth
		};

		vk::PipelineDynamicStateCreateInfo dynamicState({},
		                                                static_cast<uint32_t>(dynamicStates.size()),
		                                                dynamicStates.data());

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
			{},
			vk::PrimitiveTopology::eTriangleList,
			vk::False);

		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(swapchainExtent.width),
		                      static_cast<float>(swapchainExtent.height),
		                      0.0f,
		                      1.0f);

		vk::Rect2D scissor({}, swapchainExtent);

		vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

		vk::PipelineRasterizationStateCreateInfo rasterizer({},
		                                                    vk::False,
		                                                    vk::False,
		                                                    vk::PolygonMode::eFill,
		                                                    vk::CullModeFlagBits::eFront,
		                                                    vk::FrontFace::eCounterClockwise,
		                                                    vk::False);

		vk::PipelineMultisampleStateCreateInfo multisampling(
			{},
			vk::SampleCountFlagBits::e1,
			vk::False);

		vk::PipelineColorBlendAttachmentState colorBlendAttachment(
			vk::False,
			{},
			{},
			{},
			{},
			{},
			{},
			{});

		vk::PipelineColorBlendStateCreateInfo colorBlending(
			{},
			vk::False,
			{},
			1,
			&colorBlendAttachment);

		vk::PipelineDepthStencilStateCreateInfo depthStencil({},
		                                                     vk::True,
		                                                     vk::True,
		                                                     vk::CompareOp::eLess,
		                                                     vk::False,
		                                                     vk::False,
		                                                     {},
		                                                     {},
		                                                     0.0f,
		                                                     1.0f);

		vk::GraphicsPipelineCreateInfo pipelineInfo({},
		                                            static_cast<u32>(shaderStages.size()),
		                                            shaderStages.data(),
		                                            &vertexInputInfo,
		                                            &inputAssembly,
		                                            {},
		                                            &viewportState,
		                                            &rasterizer,
		                                            &multisampling,
		                                            &depthStencil,
		                                            &colorBlending,
		                                            &dynamicState,
		                                            pipelineLayout,
		                                            renderPass,
		                                            0);

		auto [result, graphicsPipeline] = device.createGraphicsPipeline(
			{},
			pipelineInfo,
			pAllocationCallbacks);

		SASSERT_VULKAN(result)
		return graphicsPipeline;
	}

}
