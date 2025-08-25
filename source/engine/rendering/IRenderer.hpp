#pragma once
#include "GraphicsTypes.hpp"

namespace spite
{
	class IRenderDevice;
	class IRenderCommandBuffer;
	class RenderGraph;
	class ISecondaryRenderCommandBuffer;
	struct ImageViewHandle;
	struct TextureHandle;
	class NamedBufferRegistry;

	// The abstract interface for all rendering operations.
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;
		virtual IRenderDevice& getDevice() = 0;
		virtual NamedBufferRegistry& getNamedBufferRegistry() = 0;
		virtual void waitIdle() = 0;
		virtual IRenderCommandBuffer* beginFrame() = 0;
		virtual void endFrameAndSubmit(IRenderCommandBuffer& commandBuffer) = 0;
		virtual ISecondaryRenderCommandBuffer* acquireSecondaryCommandBuffer(const heap_string& passName) = 0;
		virtual void setRenderGraph(RenderGraph* renderGraph) = 0;
		virtual bool wasSwapchainRecreated() = 0;
		virtual ImageViewHandle getCurrentSwapchainImageView() const = 0;
		virtual TextureHandle getCurrentSwapchainTextureHandle() const = 0;
		virtual Format getSwapchainFormat() const = 0;
		virtual u32 getSwapchainImageCount() const = 0;
	};
}
