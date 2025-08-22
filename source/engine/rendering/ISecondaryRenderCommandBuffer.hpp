#pragma once
#include <EASTL/span.h>

#include "GraphicsDescs.hpp"
#include "RenderResourceHandles.hpp"
#include "GraphicsTypes.hpp"
#include "IResourceSetManager.hpp"

#include "base/CollectionAliases.hpp"

namespace spite
{
	// An abstract interface for a secondary command buffer.
	// It can only record draw-level commands and cannot manage render passes directly.
	class ISecondaryRenderCommandBuffer
	{
	public:
		virtual ~ISecondaryRenderCommandBuffer() = default;

		//if it was just reset
		virtual bool isFresh() = 0;

		// The begin method needs info about the render pass it will execute within.
		virtual void begin(const sbo_vector<Format>& colorAttachmentFormats, Format depthAttachmentFormat) = 0;
		virtual void end() = 0;

		virtual void setViewportAndScissor(const Rect2D& renderArea) = 0;

		virtual void bindPipeline(PipelineHandle pipeline) = 0;
		virtual void bindVertexBuffer(BufferHandle buffer, u32 binding = 0, u64 offset = 0) = 0;
		virtual void bindIndexBuffer(BufferHandle buffer, u64 offset = 0) = 0;

#if defined(SPITE_USE_DESCRIPTOR_SETS)
		virtual void bindDescriptorSets(PipelineLayoutHandle layout, u32 firstSet, eastl::span<const ResourceSetHandle> sets) = 0;
#else
		virtual void setDescriptorBufferOffsets(PipelineLayoutHandle layout, u32 firstSet,
		                                        eastl::span<const u32> setIndices, eastl::span<const u64> offsets) = 0;
		virtual void bindDescriptorBuffers(eastl::span<const BufferHandle> buffers) = 0;
#endif

		virtual void pushConstants(PipelineLayoutHandle layout, ShaderStage stage, u32 offset, u32 size, const void* data) = 0;

		virtual void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) = 0;
		virtual void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, i32 vertexOffset = 0,
		                         u32 firstInstance = 0) = 0;

		virtual void reset() = 0;
	};
}
