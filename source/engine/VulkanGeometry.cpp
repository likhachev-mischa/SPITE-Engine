#include "VulkanGeometry.hpp"

#include <EASTL/array.h>

#include "Engine/Common.hpp"

namespace spite
{
	vk::RenderPass createGeometryRenderPass(const vk::Device& device,
	                                        const vk::Format& imageFormat,
	                                        const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::AttachmentDescription colorAttachment({},
		                                          imageFormat,
		                                          vk::SampleCountFlagBits::e1,
		                                          vk::AttachmentLoadOp::eClear,
		                                          vk::AttachmentStoreOp::eStore,
		                                          vk::AttachmentLoadOp::eDontCare,
		                                          vk::AttachmentStoreOp::eDontCare,
		                                          vk::ImageLayout::eUndefined,
		                                          vk::ImageLayout::ePresentSrcKHR);

		vk::AttachmentDescription depthAttachment({},
		                                          vk::Format::eD32Sfloat,
		                                          vk::SampleCountFlagBits::e1,
		                                          vk::AttachmentLoadOp::eLoad,
		                                          vk::AttachmentStoreOp::eDontCare,
		                                          vk::AttachmentLoadOp::eLoad,
		                                          vk::AttachmentStoreOp::eDontCare,
		                                          vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal,
		                                          vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal);

		vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
		vk::AttachmentReference depthAttachmentRef(1,
		                                           vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::SubpassDescription subpass({},
		                               vk::PipelineBindPoint::eGraphics,
		                               {},
		                               {},
		                               1,
		                               &colorAttachmentRef,
		                               nullptr,
		                               &depthAttachmentRef);

		/*	vk::SubpassDescription subpass({},
			                               vk::PipelineBindPoint::eGraphics,
			                               {},
			                               {},
			                               1,
			                               &colorAttachmentRef,
			                               nullptr);*/

		vk::SubpassDependency dependency(vk::SubpassExternal,
		                                 0,
		                                 vk::PipelineStageFlagBits::eColorAttachmentOutput |
		                                 vk::PipelineStageFlagBits::eEarlyFragmentTests,
		                                 vk::PipelineStageFlagBits::eColorAttachmentOutput |
		                                 vk::PipelineStageFlagBits::eEarlyFragmentTests,
		                                 {},
		                                 vk::AccessFlagBits::eColorAttachmentWrite |
		                                 vk::AccessFlagBits::eDepthStencilAttachmentRead);

		std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
		//std::array<vk::AttachmentDescription, 1> attachments = {colorAttachment};
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

		vk::PipelineColorBlendAttachmentState colorBlendAttachment(
			vk::False,
			{},
			{},
			{},
			{},
			{},
			{},
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

		vk::PipelineColorBlendStateCreateInfo colorBlending(
			{},
			vk::False,
			{},
			1,
			&colorBlendAttachment);

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

	std::vector<vk::Framebuffer> createGeometryFramebuffers(const vk::Device& device,
	                                                        const std::vector<vk::ImageView>&
	                                                        imageViews,
	                                                        const vk::ImageView depthImageView,
	                                                        const vk::Extent2D& swapchainExtent,
	                                                        const vk::RenderPass& renderPass,
	                                                        const vk::AllocationCallbacks*
	                                                        pAllocationCallbacks)
	{
		std::vector<vk::Framebuffer> swapchainFramebuffers;
		swapchainFramebuffers.resize(imageViews.size());

		for (size_t i = 0; i < imageViews.size(); ++i)
		{
					vk::ImageView attachments[] = {imageViews[i], depthImageView};
		
					vk::FramebufferCreateInfo framebufferInfo({},
					                                          renderPass,
					                                          2,
					                                          attachments,
					                                          swapchainExtent.width,
					                                          swapchainExtent.height,
					                                          1);

			//vk::ImageView attachments[] = {imageViews[i]};

			//vk::FramebufferCreateInfo framebufferInfo({},
			//                                          renderPass,
			//                                          1,
			//                                          attachments,
			//                                          swapchainExtent.width,
			//                                          swapchainExtent.height,
			//                                          1);

			vk::Result result;
			std::tie(result, swapchainFramebuffers[i]) = device.createFramebuffer(
				framebufferInfo,
				pAllocationCallbacks);
			SASSERT_VULKAN(result)
		}

		return swapchainFramebuffers;
	}

}
