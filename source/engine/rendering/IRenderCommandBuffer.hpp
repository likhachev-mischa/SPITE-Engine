#pragma once
#include <EASTL/span.h>

#include "RenderResourceHandles.hpp"
#include "GraphicsDescs.hpp"
#include "Synchronization.hpp"

namespace spite
{
	struct ResourceSetHandle;
    class ISecondaryRenderCommandBuffer;

	// An API-agnostic wrapper for a GPU command buffer.
    class IRenderCommandBuffer
    {
    public:
        virtual ~IRenderCommandBuffer() = default;

        virtual bool isValid() const = 0;

        virtual void beginRendering(eastl::span <ImageViewHandle> colorAttachments, ImageViewHandle depthAttachment, const Rect2D& renderArea, eastl::span<ClearValue> clearValues) = 0;
        virtual void endRendering() = 0;

        virtual void bindPipeline(PipelineHandle pipeline) = 0;
        virtual void bindVertexBuffer(BufferHandle buffer, u32 binding = 0, u64 offset = 0) = 0;
        virtual void bindIndexBuffer(BufferHandle buffer, u64 offset = 0) = 0;

#if defined(SPITE_USE_DESCRIPTOR_SETS)
        virtual void bindDescriptorSets(PipelineLayoutHandle layout, u32 firstSet, eastl::span<const ResourceSetHandle> sets) = 0;
#else
        virtual void bindDescriptorBuffers(eastl::span<const BufferHandle> buffers) = 0;
        virtual void setDescriptorBufferOffsets(PipelineLayoutHandle layout, u32 firstSet, eastl::span<const u32> setIndices, eastl::span<const u64> offsets) = 0;
#endif

        virtual void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) = 0;
        virtual void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, i32 vertexOffset = 0, u32 firstInstance = 0) = 0;

        virtual void pushConstants(PipelineLayoutHandle layout, ShaderStage stage, u32 offset, u32 size, const void* data) = 0;

        virtual void executeCommands(ISecondaryRenderCommandBuffer* secondaryCmd) = 0;

        virtual void pipelineBarrier(
            eastl::span<MemoryBarrier2> memoryBarriers,
            eastl::span<BufferBarrier2> bufferBarriers,
            eastl::span<TextureBarrier2> textureBarriers
        ) = 0;

        // Other commands like barriers, copies, etc.
    };
}
