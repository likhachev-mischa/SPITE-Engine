#pragma once
#include "RenderResourceHandles.hpp"
#include "GraphicsDescs.hpp"

namespace spite
{
	class ISampler;
	class IImageView;
	class IBuffer;

	class IRenderResourceManager
	{
	public:
		virtual ~IRenderResourceManager() = default;

		// --- Public API for Buffers ---
		virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
		virtual void destroyBuffer(BufferHandle handle) = 0;
		virtual void* mapBuffer(BufferHandle handle) = 0; // Returns a pointer to the buffer's memory
		virtual void unmapBuffer(BufferHandle handle) = 0;
		virtual void updateBuffer(BufferHandle handle, const void* data, sizet size, sizet offset = 0) = 0;
		virtual IBuffer& getBuffer(BufferHandle handle) = 0;
		virtual const IBuffer& getBuffer(BufferHandle handle) const = 0;
		virtual const BufferDesc& getBufferDesc(BufferHandle handle) const = 0;

		// --- Public API for Textures/Images ---
		virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
		virtual void destroyTexture(TextureHandle handle) = 0;

		// --- Public API for Image Views ---
		virtual ImageViewHandle createImageView(const ImageViewDesc& desc) = 0;
		virtual void destroyImageView(ImageViewHandle handle) = 0;
		virtual const IImageView& getImageView(ImageViewHandle handle) const = 0;
		virtual IImageView& getImageView(ImageViewHandle handle) = 0;

		// --- Public API for Samplers ---
		virtual SamplerHandle createSampler(const SamplerDesc& desc) = 0;
		virtual void destroySampler(SamplerHandle handle) = 0;
		virtual ISampler& getSampler(SamplerHandle handle) = 0;
		virtual const ISampler& getSampler(SamplerHandle handle) const = 0;
	};
}
