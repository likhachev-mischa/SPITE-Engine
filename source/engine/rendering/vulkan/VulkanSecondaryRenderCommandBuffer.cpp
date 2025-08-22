#include "VulkanSecondaryRenderCommandBuffer.hpp"

#include "base/Assert.hpp"
#include "engine/rendering/IRenderDevice.hpp"
#include "engine/rendering/vulkan/VulkanBuffer.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanTypeMappings.hpp"

#include "engine/rendering/IPipelineLayoutCache.hpp"
#include "engine/rendering/vulkan/VulkanPipeline.hpp"
#include "engine/rendering/vulkan/VulkanPipelineLayout.hpp"
#include "engine/rendering/vulkan/VulkanRenderResourceManager.hpp"
#include "engine/rendering/vulkan/VulkanResourceSetManager.hpp"

namespace spite
{
	VulkanSecondaryRenderCommandBuffer::VulkanSecondaryRenderCommandBuffer(
		vk::CommandBuffer nativeBuffer, IRenderDevice& device)
		: m_nativeCommandBuffer(nativeBuffer), m_device(device)
	{
		SASSERT(m_nativeCommandBuffer)
	}

	bool VulkanSecondaryRenderCommandBuffer::isFresh()
	{
		return m_isFresh;
	}

	void VulkanSecondaryRenderCommandBuffer::begin(const sbo_vector<Format>& colorAttachmentFormats,
	                                               Format depthAttachmentFormat)
	{
		m_isFresh = false;

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto vkColorAttachmentFormats = makeScratchVector<vk::Format>(FrameScratchAllocator::get());
		for (const auto& format : colorAttachmentFormats)
		{
			vkColorAttachmentFormats.push_back(vulkan::to_vulkan_format(format));
		}

		vk::CommandBufferInheritanceRenderingInfo renderingInfo{};
		renderingInfo.colorAttachmentCount = static_cast<u32>(vkColorAttachmentFormats.size());
		renderingInfo.pColorAttachmentFormats = vkColorAttachmentFormats.data();
		renderingInfo.depthAttachmentFormat = vulkan::to_vulkan_format(depthAttachmentFormat);

		vk::CommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.pNext = &renderingInfo;

		vk::CommandBufferBeginInfo beginInfo = {};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue |
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		auto res = m_nativeCommandBuffer.begin(beginInfo);
		SASSERT_VULKAN(res)
	}

	void VulkanSecondaryRenderCommandBuffer::end()
	{
		auto res = m_nativeCommandBuffer.end();
		SASSERT_VULKAN(res)
	}

	void VulkanSecondaryRenderCommandBuffer::setViewportAndScissor(const Rect2D& renderArea)
	{
		const vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(renderArea.extent.width),
		                            static_cast<float>(renderArea.extent.height),
		                            0.0f, 1.0f);
		m_nativeCommandBuffer.setViewport(0, 1, &viewport);

		const vk::Rect2D scissor = vulkan::to_vulkan_rect_2d(renderArea);
		m_nativeCommandBuffer.setScissor(0, 1, &scissor);
	}

	void VulkanSecondaryRenderCommandBuffer::bindPipeline(PipelineHandle pipelineHandle)
	{
		auto& pipelineCache = static_cast<VulkanPipelineCache&>(m_device.getPipelineCache());
		VulkanPipeline& pipeline = static_cast<VulkanPipeline&>(pipelineCache.getPipeline(pipelineHandle));
		m_nativeCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
	}

	void VulkanSecondaryRenderCommandBuffer::bindVertexBuffer(BufferHandle bufferHandle, u32 binding, u64 offset)
	{
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager());
		VulkanBuffer& buffer = resourceManager.getBufferInternal(bufferHandle);
		vk::Buffer vkBuffer = buffer.get();
		m_nativeCommandBuffer.bindVertexBuffers(binding, 1, &vkBuffer, &offset);
	}

	void VulkanSecondaryRenderCommandBuffer::bindIndexBuffer(BufferHandle bufferHandle, u64 offset)
	{
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager());
		VulkanBuffer& buffer = resourceManager.getBufferInternal(bufferHandle);
		m_nativeCommandBuffer.bindIndexBuffer(buffer.get(), offset, vk::IndexType::eUint32);
	}

#if defined(SPITE_USE_DESCRIPTOR_SETS)
	void VulkanSecondaryRenderCommandBuffer::bindDescriptorSets(PipelineLayoutHandle layoutHandle, u32 firstSet, eastl::span<const ResourceSetHandle> sets)
	{
		auto& pipelineLayoutCache = m_device.getPipelineLayoutCache();
		VulkanPipelineLayout& layout = static_cast<VulkanPipelineLayout&>(pipelineLayoutCache.getPipelineLayout(layoutHandle));
		auto& resourceSetManager = static_cast<VulkanResourceSetManager&>(m_device.getResourceSetManager());

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto vkSets = makeScratchVector<vk::DescriptorSet>(FrameScratchAllocator::get());
		for (const auto& setHandle : sets)
		{
			const auto& set = static_cast<const VulkanResourceSet&>(resourceSetManager.getSet(setHandle));
			vkSets.push_back(set.descriptorSet);
		}

		m_nativeCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout.get(), firstSet, vkSets, {});
	}
#else
	void VulkanSecondaryRenderCommandBuffer::setDescriptorBufferOffsets(PipelineLayoutHandle layoutHandle, u32 firstSet,
	                                                                    eastl::span<const u32> setIndices,
	                                                                    eastl::span<const u64> offsets)
	{
		auto& pipelineLayoutCache = m_device.getPipelineLayoutCache();
		VulkanPipelineLayout& layout = static_cast<VulkanPipelineLayout&>(pipelineLayoutCache.getPipelineLayout(
			layoutHandle));
		auto& renderContext = (static_cast<VulkanRenderDevice*>(&m_device))->getContext();

		renderContext.pfnCmdSetDescriptorBufferOffsetsEXT(m_nativeCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		                                                  layout.get(), firstSet, static_cast<u32>(setIndices.size()),
		                                                  setIndices.data(), offsets.data());
	}

	void VulkanSecondaryRenderCommandBuffer::bindDescriptorBuffers(eastl::span<const BufferHandle> buffers)
	{
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager());
		auto& renderContext = static_cast<VulkanRenderDevice&>(m_device).getContext();
		auto marker = FrameScratchAllocator::get().get_scoped_marker();

		auto descriptorBufferInfos = makeScratchVector<VkDescriptorBufferBindingInfoEXT>(FrameScratchAllocator::get());
		for (u32 i = 0; i < buffers.size(); ++i)
		{
			VulkanBuffer& buffer = static_cast<VulkanBuffer&>(resourceManager.getBuffer(buffers[i]));
			VkDescriptorBufferBindingInfoEXT bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			bufferInfo.pNext = nullptr;
			bufferInfo.address = buffer.getDeviceAddress();
			bufferInfo.usage = static_cast<VkBufferUsageFlags>(vulkan::to_vulkan_buffer_usage(
				resourceManager.getBufferDesc(buffers[i]).usage));

			descriptorBufferInfos.push_back(bufferInfo);
		}

		renderContext.pfnCmdBindDescriptorBuffersEXT(m_nativeCommandBuffer,
		                                             static_cast<u32>(descriptorBufferInfos.size()),
		                                             descriptorBufferInfos.data());
	}
#endif

	void VulkanSecondaryRenderCommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex,
	                                              u32 firstInstance)
	{
		m_nativeCommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanSecondaryRenderCommandBuffer::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex,
	                                                     i32 vertexOffset, u32 firstInstance)
	{
		m_nativeCommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VulkanSecondaryRenderCommandBuffer::reset()
	{
		auto res = m_nativeCommandBuffer.reset();
		SASSERT_VULKAN(res)
		m_isFresh = true;
	}

	vk::CommandBuffer VulkanSecondaryRenderCommandBuffer::getNativeHandle() const
	{
		return m_nativeCommandBuffer;
	}

	void VulkanSecondaryRenderCommandBuffer::pushConstants(PipelineLayoutHandle layoutHandle, ShaderStage stage,
	                                                       u32 offset, u32 size, const void* data)
	{
		auto& pipelineLayoutCache = m_device.getPipelineLayoutCache();
		VulkanPipelineLayout& layout = static_cast<VulkanPipelineLayout&>(pipelineLayoutCache.getPipelineLayout(
			layoutHandle));
		m_nativeCommandBuffer.pushConstants(
			layout.get(),
			vulkan::to_vulkan_shader_stage(stage),
			offset,
			size,
			data
		);
	}
}
