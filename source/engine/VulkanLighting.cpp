#include "VulkanLighting.hpp"

#include "Base/Platform.hpp"

#include "engine/Common.hpp"

namespace spite
{
	vk::RenderPass createLightRenderPass(const vk::Device device,
	                                     const vk::Format swapchainImageFormat,
	                                     const vk::AllocationCallbacks& allocationCallbacks)
	{
		vk::AttachmentDescription colorAttachment({},
		                                          swapchainImageFormat,
		                                          vk::SampleCountFlagBits::e1,
		                                          vk::AttachmentLoadOp::eClear,
		                                          vk::AttachmentStoreOp::eStore,
		                                          vk::AttachmentLoadOp::eDontCare,
		                                          vk::AttachmentStoreOp::eDontCare,
		                                          vk::ImageLayout::eUndefined,
		                                          vk::ImageLayout::ePresentSrcKHR);

		//GBuffer attachments
		std::array<vk::AttachmentDescription, 3> inputAttachments;
		for (sizet i = 0; i < inputAttachments.size(); ++i)
		{
			inputAttachments[i] = vk::AttachmentDescription(
				{},
				{},
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eLoad,
				vk::AttachmentStoreOp::eDontCare,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eShaderReadOnlyOptimal,
				vk::ImageLayout::eShaderReadOnlyOptimal);
		}
		inputAttachments[0].format = vk::Format::eR16G16B16A16Sfloat;
		inputAttachments[1].format = vk::Format::eR16G16B16A16Sfloat;
		inputAttachments[2].format = vk::Format::eR8G8B8A8Unorm;

		vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

		std::array<vk::AttachmentReference, 3> inputAttachmentRefs;
		for (u32 i = 0; i < inputAttachmentRefs.size(); ++i)
		{
			inputAttachmentRefs[i].attachment = i + 1;
			inputAttachmentRefs[i].layout = vk::ImageLayout::eShaderReadOnlyOptimal;
		}

		vk::SubpassDescription subpass({},
		                               vk::PipelineBindPoint::eGraphics,
		                               static_cast<u32>(inputAttachmentRefs.size()),
		                               inputAttachmentRefs.data(),
		                               1,
		                               &colorAttachmentRef);

		std::array<vk::AttachmentDescription, 4> attachments = {
			colorAttachment, inputAttachments[0], inputAttachments[1], inputAttachments[2]
		};

		vk::SubpassDependency subpassDependency(vk::SubpassExternal,
		                                        0,
		                                        vk::PipelineStageFlagBits::eFragmentShader,
		                                        vk::PipelineStageFlagBits::eFragmentShader,
		                                        vk::AccessFlagBits::eShaderRead,
		                                        vk::AccessFlagBits::eShaderRead,
		                                        vk::DependencyFlagBits::eByRegion);

		vk::RenderPassCreateInfo renderPassInfo({},
		                                        static_cast<u32>(attachments.size()),
		                                        attachments.data(),
		                                        1,
		                                        &subpass,
		                                        1,
		                                        &subpassDependency);

		auto [result, renderPass] = device.createRenderPass(renderPassInfo, &allocationCallbacks);
		SASSERT_VULKAN(result)

		return renderPass;
	}

	vk::Pipeline createLightPipeline(const vk::Device& device,
	                                 const vk::PipelineLayout& pipelineLayout,
	                                 const vk::Extent2D& extent,
	                                 const vk::RenderPass& renderPass,
	                                 const std::vector<vk::PipelineShaderStageCreateInfo>&
	                                 shaderStages,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		std::array dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor,vk::DynamicState::eLineWidth};

		vk::PipelineDynamicStateCreateInfo dynamicState({},
		                                                static_cast<uint32_t>(dynamicStates.size()),
		                                                dynamicStates.data());

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 0, nullptr, 0, nullptr);

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
			{},
			vk::PrimitiveTopology::eTriangleList,
			vk::False);

		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(extent.width),
		                      static_cast<float>(extent.height),
		                      0.0f,
		                      1.0f);

		vk::Rect2D scissor({}, extent);

		vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

		vk::PipelineRasterizationStateCreateInfo rasterizer({},
		                                                    vk::False,
		                                                    vk::False,
		                                                    vk::PolygonMode::eFill,
		                                                    vk::CullModeFlagBits::eNone,
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
		                                                     vk::False,
		                                                     vk::False,
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

	void setupLightDescriptors(const vk::Device device,
	                            const std::array<vk::ImageView, 3>& imageViews,
	                            const vk::Sampler sampler,
	                            const vk::DescriptorSet descriptorSet)
	{
		std::array<vk::DescriptorImageInfo, 3> imageInfos;

		for (u32 i = 0; i < imageInfos.size(); ++i)
		{
			imageInfos[i] = vk::DescriptorImageInfo(sampler,
			                                        imageViews[i],
			                                        vk::ImageLayout::eShaderReadOnlyOptimal);
		}

		std::array<vk::WriteDescriptorSet, 3> descriptorWrites;

		for (u32 i = 0; i< descriptorWrites.size();++i)
		{
			descriptorWrites[i] = vk::WriteDescriptorSet(descriptorSet, i, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfos[i]);
		}

		device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	void recordLightCommandBuffer(const vk::CommandBuffer& commandBuffer,
		const vk::RenderPass renderPass,
		const vk::Framebuffer framebuffer,
		const vk::Pipeline& pipeline,
		const vk::PipelineLayout& pipelineLayout,
		const std::vector<vk::DescriptorSet>& descriptorSets,
		const vk::Extent2D& swapchainExtent,
		const u32* dynamicOffsets)
	{
		vk::Result result = commandBuffer.reset();
		SASSERT_VULKAN(result)

		vk::CommandBufferInheritanceInfo inheritanceInfo(renderPass, 0, framebuffer);
		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue,
		                                     &inheritanceInfo);

		result = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(result)

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                                 pipelineLayout,
		                                 0,
		                                 descriptorSets.size(),
		                                 descriptorSets.data(),
		                                 0,
		                                 dynamicOffsets);


		vk::Rect2D renderArea({}, swapchainExtent);
		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(swapchainExtent.width),
		                      static_cast<float>(swapchainExtent.height),
		                      0.0f,
		                      1.0f);
		commandBuffer.setViewport(0, 1, &viewport);


		commandBuffer.setScissor(0, 1, &renderArea);

		commandBuffer.draw(3, 1, 0,0);

		result = commandBuffer.end();
		SASSERT_VULKAN(result)
	}
}
