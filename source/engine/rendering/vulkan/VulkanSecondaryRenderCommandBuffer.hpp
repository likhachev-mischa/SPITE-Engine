#pragma once

#include "base/VulkanUsage.hpp"

#include "engine/rendering/ISecondaryRenderCommandBuffer.hpp"

namespace spite
{
	class IRenderDevice;

	class VulkanSecondaryRenderCommandBuffer : public ISecondaryRenderCommandBuffer
	{
	public:
		VulkanSecondaryRenderCommandBuffer(vk::CommandBuffer nativeBuffer, IRenderDevice& device);
		~VulkanSecondaryRenderCommandBuffer() override = default;

		VulkanSecondaryRenderCommandBuffer(const VulkanSecondaryRenderCommandBuffer& other) = delete;
		VulkanSecondaryRenderCommandBuffer(VulkanSecondaryRenderCommandBuffer&& other) noexcept = delete;
		VulkanSecondaryRenderCommandBuffer& operator=(const VulkanSecondaryRenderCommandBuffer& other) = delete;
		VulkanSecondaryRenderCommandBuffer& operator=(VulkanSecondaryRenderCommandBuffer&& other) noexcept = delete;

		bool isFresh() override;

		void begin(const sbo_vector<Format>& colorAttachmentFormats, Format depthAttachmentFormat) override;
		void end() override;

		void setViewportAndScissor(const Rect2D& renderArea) override;

		void bindPipeline(PipelineHandle pipeline) override;
		void bindVertexBuffer(BufferHandle buffer, u32 binding = 0, u64 offset = 0) override;

		void bindIndexBuffer(BufferHandle buffer, u64 offset = 0) override;
#if defined(SPITE_USE_DESCRIPTOR_SETS)
		void bindDescriptorSets(PipelineLayoutHandle layout, u32 firstSet,
		                                eastl::span<const ResourceSetHandle> sets) override;
#else
		void setDescriptorBufferOffsets(PipelineLayoutHandle layout, u32 firstSet,
		                                        eastl::span<const u32> setIndices, eastl::span<const u64> offsets) = 0;
		void bindDescriptorBuffers(eastl::span<const BufferHandle> buffers) override;
#endif
		void pushConstants(PipelineLayoutHandle layout, ShaderStage stage, u32 offset, u32 size,
		                   const void* data) override;

		void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) override;
		void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, i32 vertexOffset = 0,
		                 u32 firstInstance = 0) override;

		void reset() override;

		vk::CommandBuffer getNativeHandle() const;

	private:
		bool m_isFresh = true;
		vk::CommandBuffer m_nativeCommandBuffer;
		IRenderDevice& m_device;
	};
}
