#include "VulkanGeometry.hpp"

#include <EASTL/array.h>

#include "Engine/Common.hpp"

namespace spite
{
	//hardcoded attachments
	vk::RenderPass createGeometryRenderPass(const vk::Device& device,
	                                        const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		//vk::AttachmentDescription colorAttachment({},
		//                                          imageFormat,
		//                                          vk::SampleCountFlagBits::e1,
		//                                          vk::AttachmentLoadOp::eClear,
		//                                          vk::AttachmentStoreOp::eStore,
		//                                          vk::AttachmentLoadOp::eDontCare,
		//                                          vk::AttachmentStoreOp::eDontCare,
		//                                          vk::ImageLayout::eUndefined,
		//                                          vk::ImageLayout::ePresentSrcKHR);

		//vk::AttachmentDescription depthAttachment({},
		//                                          vk::Format::eD32Sfloat,
		//                                          vk::SampleCountFlagBits::e1,
		//                                          vk::AttachmentLoadOp::eLoad,
		//                                          vk::AttachmentStoreOp::eDontCare,
		//                                          vk::AttachmentLoadOp::eLoad,
		//                                          vk::AttachmentStoreOp::eDontCare,
		//                                          vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal,
		//                                          vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal);

		//vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
		//vk::AttachmentReference depthAttachmentRef(1,
		//                                           vk::ImageLayout::eDepthStencilAttachmentOptimal);


		std::array<vk::AttachmentDescription, 4> attachments;
		for (sizet i = 0; i < attachments.size() - 1; ++i)
		{
			attachments[i] = vk::AttachmentDescription({},
			                                           {},
			                                           vk::SampleCountFlagBits::e1,
			                                           vk::AttachmentLoadOp::eClear,
			                                           vk::AttachmentStoreOp::eStore,
			                                           vk::AttachmentLoadOp::eDontCare,
			                                           vk::AttachmentStoreOp::eDontCare,
			                                           vk::ImageLayout::eUndefined,
			                                           vk::ImageLayout::eShaderReadOnlyOptimal);
		}

		attachments[0].format = vk::Format::eR16G16B16A16Sfloat;
		attachments[1].format = vk::Format::eR16G16B16A16Sfloat;
		attachments[2].format = vk::Format::eR8G8B8A8Unorm;

		attachments[3] = vk::AttachmentDescription({},
		                                           vk::Format::eD32Sfloat,
		                                           vk::SampleCountFlagBits::e1,
		                                           vk::AttachmentLoadOp::eLoad,
		                                           vk::AttachmentStoreOp::eStore,
		                                           vk::AttachmentLoadOp::eLoad,
		                                           vk::AttachmentStoreOp::eDontCare,
		                                           vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal,
		                                           vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal);

		std::array<vk::AttachmentReference, 3> colorAttachmentRefs{};
		colorAttachmentRefs[0] = vk::AttachmentReference(0,
		                                                 vk::ImageLayout::eColorAttachmentOptimal);
		colorAttachmentRefs[1] = vk::AttachmentReference(1,
		                                                 vk::ImageLayout::eColorAttachmentOptimal);
		colorAttachmentRefs[2] = vk::AttachmentReference(2,
		                                                 vk::ImageLayout::eColorAttachmentOptimal);

		vk::AttachmentReference depthAttachmentRef(3,
		                                           vk::ImageLayout::eDepthStencilReadOnlyOptimal);


		vk::SubpassDescription subpass({},
		                               vk::PipelineBindPoint::eGraphics,
		                               {},
		                               {},
		                               static_cast<u32>(colorAttachmentRefs.size()),
		                               colorAttachmentRefs.data(),
		                               nullptr,
		                               &depthAttachmentRef);

		std::array<vk::SubpassDependency, 2> dependencies;
		dependencies[0] = vk::SubpassDependency(vk::SubpassExternal,
		                                        0,
		                                        vk::PipelineStageFlagBits::eBottomOfPipe,
		                                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                        vk::AccessFlagBits::eMemoryRead,
		                                        vk::AccessFlagBits::eColorAttachmentWrite,
		                                        vk::DependencyFlagBits::eByRegion);

		dependencies[1] = vk::SubpassDependency(0,
		                                        vk::SubpassExternal,
		                                        vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                        vk::PipelineStageFlagBits::eFragmentShader,
		                                        vk::AccessFlagBits::eColorAttachmentWrite,
		                                        vk::AccessFlagBits::eShaderRead,
		                                        vk::DependencyFlagBits::eByRegion);

		vk::RenderPassCreateInfo renderPassInfo({},
		                                        static_cast<u32>(attachments.size()),
		                                        attachments.data(),
		                                        1,
		                                        &subpass,
		                                        static_cast<u32>(dependencies.size()),
		                                        dependencies.data());

		/*	vk::SubpassDescription subpass({},
			                               vk::PipelineBindPoint::eGraphics,
			                               {},
			                               {},
			                               1,
			                               &colorAttachmentRef,
			                               nullptr);*/

		//vk::SubpassDependency dependency(vk::SubpassExternal,
		//                                 0,
		//                                 vk::PipelineStageFlagBits::eColorAttachmentOutput |
		//                                 vk::PipelineStageFlagBits::eEarlyFragmentTests,
		//                                 vk::PipelineStageFlagBits::eColorAttachmentOutput |
		//                                 vk::PipelineStageFlagBits::eEarlyFragmentTests,
		//                                 {},
		//                                 vk::AccessFlagBits::eColorAttachmentWrite |
		//                                 vk::AccessFlagBits::eDepthStencilAttachmentRead);

		//std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
		////std::array<vk::AttachmentDescription, 1> attachments = {colorAttachment};
		//vk::RenderPassCreateInfo renderPassInfo({},
		//                                        static_cast<uint32_t>(attachments.size()),
		//                                        attachments.data(),
		//                                        1,
		//                                        &subpass,
		//                                        1,
		//                                        &dependency);


		auto [result, renderPass] = device.createRenderPass(renderPassInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return renderPass;
	}


	//TODO: add pipeline cache 
	vk::Pipeline createGeometryPipeline(const vk::Device& device,
	                                    const vk::PipelineLayout& pipelineLayout,
	                                    const vk::Extent2D& swapchainExtent,
	                                    const vk::RenderPass& renderPass,
	                                    const std::vector<vk::PipelineShaderStageCreateInfo>&
	                                    shaderStages,
	                                    const vk::PipelineVertexInputStateCreateInfo&
	                                    vertexInputInfo,
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

		//vk::PipelineColorBlendAttachmentState colorBlendAttachment(
		//	vk::False,
		//	{},
		//	{},
		//	{},
		//	{},
		//	{},
		//	{},
		//	vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		//	vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

		//vk::PipelineColorBlendStateCreateInfo colorBlending(
		//	{},
		//	vk::False,
		//	{},
		//	1,
		//	&colorBlendAttachment);

				// Define blend attachment state for each color attachment
		// Assuming all 3 G-buffer attachments have the same blend state (no blending)
		vk::PipelineColorBlendAttachmentState colorBlendAttachmentState(
			vk::False, // blendEnable
			{}, // srcColorBlendFactor
			{}, // dstColorBlendFactor
			{}, // colorBlendOp
			{}, // srcAlphaBlendFactor
			{}, // dstAlphaBlendFactor
			{}, // alphaBlendOp
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

		std::array<vk::PipelineColorBlendAttachmentState, 3> colorBlendAttachments;
		colorBlendAttachments[0] = colorBlendAttachmentState;
		colorBlendAttachments[1] = colorBlendAttachmentState;
		colorBlendAttachments[2] = colorBlendAttachmentState;

		vk::PipelineColorBlendStateCreateInfo colorBlending(
			{},
			vk::False, // logicOpEnable
			{},        // logicOp
			static_cast<u32>(colorBlendAttachments.size()), // attachmentCount, now 3
			colorBlendAttachments.data()); // pAttachments

		vk::PipelineDepthStencilStateCreateInfo depthStencil({},
		                                                     vk::True,
		                                                     vk::False,
		                                                     vk::CompareOp::eLessOrEqual,
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
