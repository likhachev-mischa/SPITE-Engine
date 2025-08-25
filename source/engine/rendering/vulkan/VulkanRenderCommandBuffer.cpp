#include "VulkanRenderCommandBuffer.hpp"

#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"
#include "VulkanImageView.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanPipelineCache.hpp"
#include "VulkanPipelineLayout.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanSecondaryRenderCommandBuffer.hpp"
#include "VulkanRenderResourceManager.hpp"
#include "VulkanResourceSetManager.hpp"
#include "VulkanTypeMappings.hpp"

#include "base/Assert.hpp"

namespace spite
{
	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(vk::CommandBuffer nativeBuffer, VulkanRenderDevice* device)
		: m_nativeCommandBuffer(nativeBuffer),
		  m_device(device)
	{
		SASSERT(VulkanRenderCommandBuffer::isValid())
	}

	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(VulkanRenderCommandBuffer&& other) noexcept:
		m_nativeCommandBuffer(other.m_nativeCommandBuffer),
		m_device(other.m_device)
	{
		SASSERT(VulkanRenderCommandBuffer::isValid())
	}

	void VulkanRenderCommandBuffer::beginRendering(eastl::span<ImageViewHandle> colorAttachments,
	                                               ImageViewHandle depthAttachment, const Rect2D& renderArea,
	                                               eastl::span<ClearValue> clearValues)
	{
		SASSERT(isValid())
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device->getResourceManager());
		auto marker = FrameScratchAllocator::get().get_scoped_marker();

		auto vkColorAttachments = makeScratchVector<vk::RenderingAttachmentInfo>(FrameScratchAllocator::get());
		u32 clearValueIndex = 0;
		for (const auto& handle : colorAttachments)
		{
			VulkanImageView& imageView = resourceManager.getImageViewInternal(handle);
			vk::RenderingAttachmentInfo attachmentInfo{};
			attachmentInfo.imageView = imageView.get();
			attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
			attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			if (clearValueIndex < clearValues.size() && std::holds_alternative<ClearColorValue>(
				clearValues[clearValueIndex]))
			{
				const auto& color = std::get<ClearColorValue>(clearValues[clearValueIndex]);
				attachmentInfo.clearValue.color = vk::ClearColorValue{
					std::array<float, 4>{color.r, color.g, color.b, color.a}
				};
				clearValueIndex++;
			}
			vkColorAttachments.push_back(attachmentInfo);
		}

		vk::RenderingAttachmentInfo depthAttachmentInfo{};
		if (depthAttachment.isValid())
		{
			VulkanImageView& imageView = resourceManager.getImageViewInternal(depthAttachment);
			depthAttachmentInfo.imageView = imageView.get();
			depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
			depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			if (clearValueIndex < clearValues.size() && std::holds_alternative<ClearDepthStencilValue>(
				clearValues[clearValueIndex]))
			{
				const auto& ds = std::get<ClearDepthStencilValue>(clearValues[clearValueIndex]);
				depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue{ds.depth, ds.stencil};
			}
			else
			{
				depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
			}
		}

		vk::RenderingInfo renderingInfo{};
		renderingInfo.flags = vk::RenderingFlagBits::eContentsSecondaryCommandBuffers | vk::RenderingFlagBits::eContentsInlineEXT;
		renderingInfo.renderArea = vulkan::to_vulkan_rect_2d(renderArea);
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<u32>(vkColorAttachments.size());
		renderingInfo.pColorAttachments = vkColorAttachments.data();
		if (depthAttachment.isValid())
		{
			renderingInfo.pDepthAttachment = &depthAttachmentInfo;
		}

		m_nativeCommandBuffer.beginRendering(renderingInfo);
	}

	void VulkanRenderCommandBuffer::endRendering()
	{
		SASSERT(isValid())
		m_nativeCommandBuffer.endRendering();
	}

	void VulkanRenderCommandBuffer::bindPipeline(PipelineHandle pipelineHandle)
	{
		SASSERT(isValid())
		auto& pipelineCache = m_device->getPipelineCache();
		VulkanPipeline& pipeline = static_cast<VulkanPipeline&>(pipelineCache.getPipeline(pipelineHandle));
		m_nativeCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
	}

	void VulkanRenderCommandBuffer::bindVertexBuffer(BufferHandle bufferHandle, u32 binding, u64 offset)
	{
		SASSERT(isValid())
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device->getResourceManager());
		VulkanBuffer& buffer = resourceManager.getBufferInternal(bufferHandle);
		vk::Buffer vkBuffer = buffer.get();
		m_nativeCommandBuffer.bindVertexBuffers(binding, 1, &vkBuffer, &offset);
	}

	void VulkanRenderCommandBuffer::bindIndexBuffer(BufferHandle bufferHandle, u64 offset)
	{
		SASSERT(isValid())
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device->getResourceManager());
		VulkanBuffer& buffer = resourceManager.getBufferInternal(bufferHandle);
		m_nativeCommandBuffer.bindIndexBuffer(buffer.get(), offset, vk::IndexType::eUint32);
	}

#if defined(SPITE_USE_DESCRIPTOR_SETS)
	void VulkanRenderCommandBuffer::bindDescriptorSets(PipelineLayoutHandle layoutHandle, u32 firstSet, eastl::span<const ResourceSetHandle> sets)
	{
		SASSERT(isValid())
		auto& pipelineLayoutCache = m_device->getPipelineLayoutCache();
		VulkanPipelineLayout& layout = static_cast<VulkanPipelineLayout&>(pipelineLayoutCache.getPipelineLayout(layoutHandle));
		auto& resourceSetManager = static_cast<VulkanResourceSetManager&>(m_device->getResourceSetManager());

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
	void VulkanRenderCommandBuffer::bindDescriptorBuffers(eastl::span<const BufferHandle> buffers)
	{
		SASSERT(isValid());
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device->getResourceManager());
		auto marker = FrameScratchAllocator::get().get_scoped_marker();

		auto descriptorBufferInfos = makeScratchVector<
			VkDescriptorBufferBindingInfoEXT>(FrameScratchAllocator::get());
		for (u32 i = 0; i < buffers.size(); ++i)
		{
			VulkanBuffer& buffer = resourceManager.getBufferInternal(buffers[i]);
			VkDescriptorBufferBindingInfoEXT bufferInfo;
			bufferInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			bufferInfo.address = buffer.getDeviceAddress();
			bufferInfo.usage = static_cast<VkBufferUsageFlags>(vulkan::to_vulkan_buffer_usage(
				resourceManager.getBufferDesc(buffers[i]).usage));
			bufferInfo.pNext = nullptr;

			descriptorBufferInfos.push_back(bufferInfo);
		}

		m_device->getContext().pfnCmdBindDescriptorBuffersEXT(m_nativeCommandBuffer,
		                                                      static_cast<u32>(descriptorBufferInfos.size()),
		                                                      descriptorBufferInfos.data());
	}

	void VulkanRenderCommandBuffer::setDescriptorBufferOffsets(PipelineLayoutHandle layoutHandle, u32 firstSet,
	                                                           eastl::span<const u32> setIndices,
	                                                           eastl::span<const u64> offsets)
	{
		SASSERT(isValid());
		auto& pipelineLayoutCache = m_device->getPipelineLayoutCache();
		VulkanPipelineLayout& layout = static_cast<VulkanPipelineLayout&>(pipelineLayoutCache.getPipelineLayout(
			layoutHandle));

		m_device->getContext().pfnCmdSetDescriptorBufferOffsetsEXT(m_nativeCommandBuffer,
		                                                           static_cast<VkPipelineBindPoint>(
			                                                           vk::PipelineBindPoint::eGraphics), layout.get(),
		                                                           firstSet,
		                                                           static_cast<u32>(setIndices.size()),
		                                                           setIndices.data(), offsets.data());
	}
#endif

	void VulkanRenderCommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
	{
		SASSERT(isValid())
		m_nativeCommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanRenderCommandBuffer::pipelineBarrier(eastl::span<MemoryBarrier2> memoryBarriers,
	                                                eastl::span<BufferBarrier2> bufferBarriers,
	                                                eastl::span<TextureBarrier2> textureBarriers)
	{
		SASSERT(isValid())
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device->getResourceManager());
		auto marker = FrameScratchAllocator::get().get_scoped_marker();

		auto vkMemoryBarriers = makeScratchVector<vk::MemoryBarrier2>(FrameScratchAllocator::get());
		for (const auto& barrier : memoryBarriers)
		{
			vkMemoryBarriers.push_back(vk::MemoryBarrier2(
				vulkan::to_vulkan_pipeline_stage2(barrier.srcStageMask),
				vulkan::to_vulkan_access_flags2(barrier.srcAccessMask),
				vulkan::to_vulkan_pipeline_stage2(barrier.dstStageMask),
				vulkan::to_vulkan_access_flags2(barrier.dstAccessMask)
			));
		}

		auto vkBufferBarriers = makeScratchVector<vk::BufferMemoryBarrier2>(FrameScratchAllocator::get());
		for (const auto& barrier : bufferBarriers)
		{
			VulkanBuffer& buffer = resourceManager.getBufferInternal(barrier.buffer);
			vkBufferBarriers.push_back(vk::BufferMemoryBarrier2(
				vulkan::to_vulkan_pipeline_stage2(barrier.srcStageMask),
				vulkan::to_vulkan_access_flags2(barrier.srcAccessMask),
				vulkan::to_vulkan_pipeline_stage2(barrier.dstStageMask),
				vulkan::to_vulkan_access_flags2(barrier.dstAccessMask),
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				buffer.get(),
				barrier.offset,
				barrier.size
			));
		}

		auto vkImageBarriers = makeScratchVector<vk::ImageMemoryBarrier2>(FrameScratchAllocator::get());
		for (const auto& barrier : textureBarriers)
		{
			VulkanImage& image = resourceManager.getTexture(barrier.texture);
			const TextureDesc& desc = resourceManager.getTextureDesc(barrier.texture);
			vk::ImageAspectFlags aspectMask = vulkan::to_vulkan_aspect_mask(desc.format);

			vkImageBarriers.push_back(vk::ImageMemoryBarrier2(
				vulkan::to_vulkan_pipeline_stage2(barrier.srcStageMask),
				vulkan::to_vulkan_access_flags2(barrier.srcAccessMask),
				vulkan::to_vulkan_pipeline_stage2(barrier.dstStageMask),
				vulkan::to_vulkan_access_flags2(barrier.dstAccessMask),
				vulkan::to_vulkan_image_layout(barrier.oldLayout),
				vulkan::to_vulkan_image_layout(barrier.newLayout),
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				image.get(),
				vk::ImageSubresourceRange(aspectMask, barrier.baseMipLevel, barrier.levelCount,
				                          barrier.baseArrayLayer, barrier.layerCount)
			));
		}

		vk::DependencyInfo dependencyInfo{};
		dependencyInfo.memoryBarrierCount = static_cast<u32>(vkMemoryBarriers.size());
		dependencyInfo.pMemoryBarriers = vkMemoryBarriers.data();
		dependencyInfo.bufferMemoryBarrierCount = static_cast<u32>(vkBufferBarriers.size());
		dependencyInfo.pBufferMemoryBarriers = vkBufferBarriers.data();
		dependencyInfo.imageMemoryBarrierCount = static_cast<u32>(vkImageBarriers.size());
		dependencyInfo.pImageMemoryBarriers = vkImageBarriers.data();

		m_nativeCommandBuffer.pipelineBarrier2(dependencyInfo);
	}

	void VulkanRenderCommandBuffer::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset,
	                                            u32 firstInstance)
	{
		SASSERT(isValid())
		m_nativeCommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	vk::CommandBuffer VulkanRenderCommandBuffer::getNativeHandle() const
	{
		SASSERT(isValid())
		return m_nativeCommandBuffer;
	}

	VulkanRenderCommandBuffer& VulkanRenderCommandBuffer::operator=(VulkanRenderCommandBuffer&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		m_nativeCommandBuffer = other.m_nativeCommandBuffer;
		m_device = other.m_device;

		return *this;
	}

	bool VulkanRenderCommandBuffer::isValid() const
	{
		return (m_nativeCommandBuffer != nullptr) && (m_device != nullptr);
	}

	void VulkanRenderCommandBuffer::executeCommands(ISecondaryRenderCommandBuffer* secondaryCmd)
	{
		SASSERT(isValid())
		auto vulkanSecCb = static_cast<VulkanSecondaryRenderCommandBuffer*>(secondaryCmd);
		auto handle = vulkanSecCb->getNativeHandle();
		m_nativeCommandBuffer.executeCommands(1, &handle);
	}

	void VulkanRenderCommandBuffer::pushConstants(PipelineLayoutHandle layoutHandle, ShaderStage stage, u32 offset,
	                                              u32 size, const void* data)
	{
		SASSERT(isValid())
		auto& pipelineLayoutCache = m_device->getPipelineLayoutCache();
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
