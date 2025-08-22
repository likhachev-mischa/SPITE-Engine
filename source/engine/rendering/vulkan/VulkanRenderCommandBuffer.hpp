#pragma once
#include "engine/rendering/IRenderCommandBuffer.hpp"
#include "base/VulkanUsage.hpp"

namespace spite
{
	class ISecondaryRenderCommandBuffer;
	class VulkanRenderDevice;

	class VulkanRenderCommandBuffer : public IRenderCommandBuffer
	{
	private:
		vk::CommandBuffer m_nativeCommandBuffer = nullptr;
		VulkanRenderDevice* m_device{};

	public:
		VulkanRenderCommandBuffer() = default;
		VulkanRenderCommandBuffer(vk::CommandBuffer nativeBuffer, VulkanRenderDevice* device);
		~VulkanRenderCommandBuffer() override = default;

		VulkanRenderCommandBuffer(const VulkanRenderCommandBuffer& other) = delete;
		VulkanRenderCommandBuffer(VulkanRenderCommandBuffer&& other) noexcept;
		VulkanRenderCommandBuffer& operator=(const VulkanRenderCommandBuffer& other) = delete;
		VulkanRenderCommandBuffer& operator=(VulkanRenderCommandBuffer&& other) noexcept;

		bool isValid() const override;

		void executeCommands(ISecondaryRenderCommandBuffer* secondaryCmd) override;

		void beginRendering(eastl::span<ImageViewHandle> colorAttachments, ImageViewHandle depthAttachment, const Rect2D& renderArea, eastl::span<ClearValue> clearValues) override;
		void endRendering() override;

		void bindPipeline(PipelineHandle pipeline) override;
		void bindVertexBuffer(BufferHandle buffer, u32 binding = 0, u64 offset = 0) override;
		void bindIndexBuffer(BufferHandle buffer, u64 offset = 0) override;
#if defined(SPITE_USE_DESCRIPTOR_SETS)
        void bindDescriptorSets(PipelineLayoutHandle layout, u32 firstSet, eastl::span<const ResourceSetHandle> sets) override;
#else
        void bindDescriptorBuffers(eastl::span<const BufferHandle> buffers) override;
        void setDescriptorBufferOffsets(PipelineLayoutHandle layout, u32 firstSet, eastl::span<const u32> setIndices, eastl::span<const u64> offsets) override;
#endif

		void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) override;
		void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, i32 vertexOffset = 0, u32 firstInstance = 0) override;

		void pushConstants(PipelineLayoutHandle layout, ShaderStage stage, u32 offset, u32 size, const void* data) override;

		void pipelineBarrier(
			eastl::span<MemoryBarrier2> memoryBarriers,
			eastl::span<BufferBarrier2> bufferBarriers,
			eastl::span<TextureBarrier2> textureBarriers
		) override;

		vk::CommandBuffer getNativeHandle() const;
	};
}
