#include "VulkanPipelineCache.hpp"

#include <memory>

#include <EASTL/array.h>

#include "VulkanPipeline.hpp"
#include "VulkanPipelineLayout.hpp"
#include "VulkanPipelineLayoutCache.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanShaderModule.hpp"
#include "VulkanShaderModuleCache.hpp"
#include "VulkanShaderReflectionModule.hpp"
#include "VulkanTypeMappings.hpp"

#include "base/Assert.hpp"
#include "base/CollectionUtilities.hpp"
#include "base/File.hpp"

namespace spite
{
	bool PipelineDescription::operator==(const PipelineDescription& other) const
	{
		return resourceSetLayouts == other.resourceSetLayouts &&
			pushConstantRanges == other.pushConstantRanges &&
			renderPass == other.renderPass &&
			shaderStages == other.shaderStages &&
			vertexBindings == other.vertexBindings &&
			vertexAttributes == other.vertexAttributes &&
			topology == other.topology &&
			polygonMode == other.polygonMode &&
			cullMode == other.cullMode &&
			frontFace == other.frontFace &&
			depthTestEnable == other.depthTestEnable &&
			depthWriteEnable == other.depthWriteEnable &&
			depthCompareOp == other.depthCompareOp &&
			blendStates == other.blendStates &&
			colorAttachmentFormats == other.colorAttachmentFormats &&
			depthAttachmentFormat == other.depthAttachmentFormat;
	}

	sizet PipelineDescription::Hash::operator()(const PipelineDescription& desc) const
	{
		sizet seed = 0;
		for (const auto& layout : desc.resourceSetLayouts)
		{
			hashCombine(seed, layout.id, layout.generation);
		}
		for (const auto& range : desc.pushConstantRanges)
		{
			hashCombine(seed, range.shaderStages, range.offset, range.size);
		}
		hashCombine(seed, desc.renderPass.id, desc.renderPass.generation);
		for (const auto& sm : desc.shaderStages)
		{
			hashCombine(seed, sm.module.id, sm.module.generation, sm.stage, sm.entryPoint);
		}
		for (const auto& binding : desc.vertexBindings)
		{
			hashCombine(seed, binding.binding, binding.stride, binding.inputRate);
		}
		for (const auto& attr : desc.vertexAttributes)
		{
			hashCombine(seed, attr.location, attr.binding, attr.format, attr.offset);
		}
		hashCombine(seed, desc.topology, desc.polygonMode, desc.cullMode, desc.frontFace,
		            desc.depthTestEnable, desc.depthWriteEnable, desc.depthCompareOp);
		for (const auto& blend : desc.blendStates)
		{
			hashCombine(seed, blend.blendEnable, blend.srcColorBlendFactor, blend.dstColorBlendFactor,
			            blend.colorBlendOp,
			            blend.srcAlphaBlendFactor, blend.dstAlphaBlendFactor, blend.alphaBlendOp);
		}
		for (const auto& format : desc.colorAttachmentFormats)
		{
			hashCombine(seed, format);
		}
		hashCombine(seed, desc.depthAttachmentFormat);
		return seed;
	}

	VulkanPipelineCache::VulkanPipelineCache(VulkanRenderDevice& device, const HeapAllocator& allocator)
		: m_device(device),
		  m_allocator(allocator),
		  m_cache(makeHeapMap<PipelineDescription, PipelineHandle, PipelineDescription::Hash>(allocator)),
		  m_pipelines(allocator),
		  m_pipelineDescriptions(makeHeapVector<PipelineDescription>(allocator))
	{
	}

	VulkanPipelineCache::~VulkanPipelineCache()
	{
	}

	PipelineHandle VulkanPipelineCache::getOrCreatePipeline(const PipelineDescription& description)
	{
		auto it = m_cache.find(description);
		if (it != m_cache.end())
		{
			return it->second;
		}

		auto& renderContext = m_device.getContext();
		auto& shaderModuleCache = m_device.getShaderModuleCache();
		auto& pipelineLayoutCache = m_device.getPipelineLayoutCache();

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto shaderStages = makeScratchVector<vk::PipelineShaderStageCreateInfo>(FrameScratchAllocator::get());
		for (const auto& stageDesc : description.shaderStages)
		{
			auto& shaderModule = shaderModuleCache.getShaderModule(stageDesc.module);
			shaderStages.push_back(vk::PipelineShaderStageCreateInfo(
				{},
				vulkan::to_vulkan_shader_stage_bit(stageDesc.stage),
				static_cast<VulkanShaderModule&>(shaderModule).get(),
				stageDesc.entryPoint.c_str()
			));
		}

		PipelineLayoutDescription layoutDesc{description.resourceSetLayouts, description.pushConstantRanges};
		PipelineLayoutHandle layoutHandle = pipelineLayoutCache.getOrCreatePipelineLayout(layoutDesc);
		VulkanPipelineLayout& pipelineLayout = static_cast<VulkanPipelineLayout&>(pipelineLayoutCache.
			getPipelineLayout(layoutHandle));

		auto vertexBindings = makeScratchVector<vk::VertexInputBindingDescription>(FrameScratchAllocator::get());
		for (const auto& bindingDesc : description.vertexBindings)
		{
			vertexBindings.push_back(vk::VertexInputBindingDescription(
				bindingDesc.binding,
				bindingDesc.stride,
				static_cast<vk::VertexInputRate>(bindingDesc.inputRate)
			));
		}

		auto vertexAttributes = makeScratchVector<vk::VertexInputAttributeDescription>(FrameScratchAllocator::get());
		for (const auto& attributeDesc : description.vertexAttributes)
		{
			vertexAttributes.push_back(vk::VertexInputAttributeDescription(
				attributeDesc.location,
				attributeDesc.binding,
				vulkan::to_vulkan_format(attributeDesc.format),
				attributeDesc.offset
			));
		}

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<u32>(vertexBindings.size());
		vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(vertexAttributes.size());
		vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.topology = vulkan::to_vulkan_primitive_topology(description.topology);
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		vk::PipelineViewportStateCreateInfo viewportState{};
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		vk::PipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = vulkan::to_vulkan_polygon_mode(description.polygonMode);
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = vulkan::to_vulkan_cull_mode(description.cullMode);
		rasterizer.frontFace = vulkan::to_vulkan_front_face(description.frontFace);
		rasterizer.depthBiasEnable = VK_FALSE;

		vk::PipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

		vk::PipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.depthTestEnable = description.depthTestEnable;
		depthStencil.depthWriteEnable = description.depthWriteEnable;
		depthStencil.depthCompareOp = vulkan::to_vulkan_compare_op(description.depthCompareOp);
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		auto colorBlendAttachments = makeScratchVector<vk::PipelineColorBlendAttachmentState>(
			FrameScratchAllocator::get());
		for (const auto& blendState : description.blendStates)
		{
			vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
			colorBlendAttachment.blendEnable = blendState.blendEnable;
			colorBlendAttachment.srcColorBlendFactor = vulkan::to_vulkan_blend_factor(blendState.srcColorBlendFactor);
			colorBlendAttachment.dstColorBlendFactor = vulkan::to_vulkan_blend_factor(blendState.dstColorBlendFactor);
			colorBlendAttachment.colorBlendOp = vulkan::to_vulkan_blend_op(blendState.colorBlendOp);
			colorBlendAttachment.srcAlphaBlendFactor = vulkan::to_vulkan_blend_factor(blendState.srcAlphaBlendFactor);
			colorBlendAttachment.dstAlphaBlendFactor = vulkan::to_vulkan_blend_factor(blendState.dstAlphaBlendFactor);
			colorBlendAttachment.alphaBlendOp = vulkan::to_vulkan_blend_op(blendState.alphaBlendOp);
			colorBlendAttachments.push_back(colorBlendAttachment);
		}

		vk::PipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = vk::LogicOp::eCopy;
		colorBlending.attachmentCount = static_cast<u32>(colorBlendAttachments.size());
		colorBlending.pAttachments = colorBlendAttachments.data();

		auto dynamicStates = eastl::array{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		vk::PipelineRenderingCreateInfo renderingCreateInfo{};

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
#if !defined(SPITE_USE_DESCRIPTOR_SETS)
		pipelineInfo.flags = vk::PipelineCreateFlagBits::eDescriptorBufferEXT;
#endif
		if (!description.renderPass.isValid()) // Check for dynamic rendering
		{
			auto colorFormats = makeScratchVector<vk::Format>(FrameScratchAllocator::get());
			for(const auto& f : description.colorAttachmentFormats) {
				colorFormats.push_back(vulkan::to_vulkan_format(f));
			}
			renderingCreateInfo.colorAttachmentCount = static_cast<u32>(colorFormats.size());
			renderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
			renderingCreateInfo.depthAttachmentFormat = vulkan::to_vulkan_format(description.depthAttachmentFormat);
			pipelineInfo.pNext = &renderingCreateInfo;
		}

		pipelineInfo.stageCount = static_cast<u32>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout.get();
		pipelineInfo.subpass = 0;

		auto newPipe = std::make_unique<VulkanPipeline>(renderContext.device, m_device.getVkPipelineCache(), layoutHandle,
		                                                pipelineInfo);

		u32 index;
		if (!m_pipelines.freeIndices.empty())
		{
			index = m_pipelines.freeIndices.back();
			m_pipelines.freeIndices.pop_back();
			m_pipelines.resources[index] = std::move(newPipe);
			m_pipelineDescriptions[index] = description;
		}
		else
		{
			index = static_cast<u32>(m_pipelines.resources.size());
			m_pipelines.resources.push_back(std::move(newPipe));
			m_pipelines.generations.push_back(0);
			m_pipelineDescriptions.push_back(description);
		}

		PipelineHandle newHandle{index, m_pipelines.generations[index]};
		m_cache[description] = newHandle;

		return newHandle;
	}

	void VulkanPipelineCache::destroyPipeline(PipelineHandle handle)
	{
		if (!handle.isValid() || handle.id >= m_pipelines.resources.size() || m_pipelines.generations[handle.id] !=
			handle.generation)
		{
			SASSERTM(false, "Attempted to destroy a pipeline with an invalid or stale handle.")
			return;
		}

		const auto& description = m_pipelineDescriptions[handle.id];
		m_cache.erase(description);

		m_pipelines.resources[handle.id].reset();
		m_pipelines.freeIndices.push_back(handle.id);
		m_pipelines.generations[handle.id]++;
	}

	IPipeline& VulkanPipelineCache::getPipeline(PipelineHandle handle)
	{
		SASSERT(
			handle.isValid() && handle.id < m_pipelines.resources.size() && m_pipelines.generations[handle.id] == handle
			.generation)
		return *m_pipelines.resources[handle.id];
	}

	const IPipeline& VulkanPipelineCache::getPipeline(PipelineHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_pipelines.resources.size() && m_pipelines.generations[handle.id] == handle
			.generation)
		return *m_pipelines.resources[handle.id];
	}

	const PipelineDescription& VulkanPipelineCache::getPipelineDescription(PipelineHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_pipelineDescriptions.size() && m_pipelines.generations[handle.id] ==
			handle
			.generation)
		return m_pipelineDescriptions[handle.id];
	}

	PipelineLayoutHandle VulkanPipelineCache::getPipelineLayoutHandle(PipelineHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_pipelines.resources.size() && m_pipelines.generations[handle.id] == handle
			.generation)
		return static_cast<VulkanPipeline&>(*m_pipelines.resources[handle.id]).getLayoutHandle();
	}

	void VulkanPipelineCache::clear()
	{
		m_cache.clear();
		for (u32 i = 0; i < m_pipelines.resources.size(); ++i)
		{
			if (m_pipelines.resources[i])
			{
				m_pipelines.resources[i].reset();
				m_pipelines.generations[i]++;
				m_pipelines.freeIndices.push_back(i);
			}
		}
	}

	PipelineHandle VulkanPipelineCache::getOrCreatePipeline(const ComputePipelineDescription& description)
	{
		SASSERTM(false, "Compute pipelines not yet implemented.")
		return {};
	}

	PipelineHandle VulkanPipelineCache::getOrCreatePipelineFromShaders(
		eastl::span<const ShaderStageDescription> shaderStages,
		const PipelineDescription& baseDescription)
	{
		auto& shaderModuleCache = m_device.getShaderModuleCache();

		// 1. Reflect all provided shader stages and merge their data
		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto mergedLayouts = makeScratchMap<u32, ResourceSetLayoutDescription>(FrameScratchAllocator::get());
		PipelineDescription finalDescription = baseDescription; // Start with the base

		sbo_vector<VertexInputAttributeDesc> reflectedAttributes;
		sbo_vector<PushConstantRange> mergedPushConstants;

		for (const auto& stageDesc : shaderStages)
		{
			ShaderModuleHandle moduleHandle = shaderModuleCache.getOrCreateShaderModule(
				{.path = stageDesc.path, .stage = stageDesc.stage}
			);
			const ShaderReflectionData& reflectionData =
				static_cast<VulkanShaderModuleCache&>(shaderModuleCache).getReflectionData(moduleHandle);

			finalDescription.shaderStages.push_back({
				.module = moduleHandle, .stage = stageDesc.stage,
				.entryPoint = stageDesc.entryPoint
			});

			if (stageDesc.stage == ShaderStage::VERTEX)
			{
				SASSERTM(reflectedAttributes.empty(),
				         "Multiple vertex shaders provided, vertex attribute reflection is ambiguous.")
				reflectedAttributes = reflectionData.vertexInputAttributes;
			}

			for (const auto& [set, layoutDesc] : reflectionData.resourceSetLayouts)
			{
				if (mergedLayouts.find(set) != mergedLayouts.end())
				{
					for (const auto& binding : layoutDesc.bindings)
					{
						auto& existingBindings = mergedLayouts[set].bindings;
						auto it = std::ranges::find_if(existingBindings,
						                               [&](const ResourceBinding& b)
						                               {
							                               return b.binding == binding.binding;
						                               });

						if (it != existingBindings.end())
						{
							it->shaderStages |= binding.shaderStages;
						}
						else
						{
							existingBindings.push_back(binding);
						}
					}
				}
				else
				{
					mergedLayouts[set] = layoutDesc;
				}
			}

			// Merge push constants
			for (const auto& reflectedRange : reflectionData.pushConstantRanges)
			{
				auto it = std::ranges::find_if(mergedPushConstants,
				                               [&](const PushConstantRange& r)
				                               {
					                               // Check for overlapping ranges
					                               return std::max(r.offset, reflectedRange.offset) < std::min(
						                               r.offset + r.size, reflectedRange.offset + reflectedRange.size);
				                               });

				if (it != mergedPushConstants.end())
				{
					// Merge stages for the same range
					it->shaderStages |= reflectedRange.shaderStages;
				}
				else
				{
					mergedPushConstants.push_back(reflectedRange);
				}
			}
		}

		// 2. Generate vertex input state from reflection
		if (!reflectedAttributes.empty())
		{
			// Sort attributes by location to ensure deterministic order
			std::ranges::sort(reflectedAttributes, {}, &VertexInputAttributeDesc::location);

			finalDescription.vertexAttributes.clear();
			u32 currentOffset = 0;
			u32 defaultBinding = 0; // Assume a single interleaved buffer at binding 0

			for (const auto& reflectedAttr : reflectedAttributes)
			{
				VertexInputAttributeDesc finalAttr;
				finalAttr.location = reflectedAttr.location;
				finalAttr.binding = defaultBinding;
				finalAttr.format = reflectedAttr.format;
				finalAttr.offset = currentOffset;
				finalDescription.vertexAttributes.push_back(finalAttr);
				currentOffset += vulkan::get_format_size(reflectedAttr.format);
			}

			// If user hasn't defined a binding, create a default one with the calculated stride.
			if (finalDescription.vertexBindings.empty())
			{
				finalDescription.vertexBindings.push_back({.binding = defaultBinding, .stride = currentOffset});
			}
			else
			{
				// If they defined a binding, assume they know what they're doing and just set the stride if it's 0.
				auto it = std::ranges::find_if(finalDescription.vertexBindings,
				                               [&](const auto& b) { return b.binding == defaultBinding; });
				if (it != finalDescription.vertexBindings.end() && it->stride == 0)
				{
					it->stride = currentOffset;
				}
			}
		}

		// 3. Create the final layout handles
		auto& resourceSetLayoutCache = m_device.getResourceSetLayoutCache();
		for (const auto& [set, layoutDesc] : mergedLayouts)
		{
			finalDescription.resourceSetLayouts.push_back(
				resourceSetLayoutCache.getOrCreateLayout(layoutDesc)
			);
		}
		finalDescription.pushConstantRanges = std::move(mergedPushConstants);

		// 4. Create the pipeline
		return getOrCreatePipeline(finalDescription);
	}
}
