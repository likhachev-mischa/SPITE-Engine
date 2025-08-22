#pragma once
#include "GraphicsDescs.hpp"

namespace spite
{
	struct PipelineLayoutDescription;
	struct ResourceSetLayoutDescription;
	struct ShaderDescription;
	struct PipelineDescription;
	class IRenderResourceManager;
	class IPipelineCache;
	class IShaderModuleCache;
	class IResourceSetLayoutCache;
	class IPipelineLayoutCache;
	class IResourceSetManager;

	// The RenderDevice is the central hub for creating and managing all GPU resources.
	// It owns all the underlying resource managers and caches, providing a single,
	// unified interface to the rest of the engine.
	class IRenderDevice
	{
	public:
		virtual ~IRenderDevice() = default;

		// --- Resource Management ---
		virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
		virtual void destroyBuffer(BufferHandle handle) = 0;

		virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
		virtual void destroyTexture(TextureHandle handle) = 0;

		virtual ImageViewHandle createImageView(const ImageViewDesc& desc) = 0;
		virtual void destroyImageView(ImageViewHandle handle) = 0;

		virtual SamplerHandle createSampler(const SamplerDesc& desc) = 0;
		virtual void destroySampler(SamplerHandle handle) = 0;

		// --- Cache Access ---
		virtual PipelineHandle getOrCreatePipeline(const PipelineDescription& description) = 0;
		virtual ShaderModuleHandle getOrCreateShaderModule(const ShaderDescription& description) = 0;
		virtual ResourceSetLayoutHandle getOrCreateResourceSetLayout(const ResourceSetLayoutDescription& description) =
		0;
		virtual PipelineLayoutHandle getOrCreatePipelineLayout(const PipelineLayoutDescription& description) = 0;

		// --- Direct Manager Access (for internal use, e.g., by Renderer) ---
		virtual IRenderResourceManager& getResourceManager() = 0;
		virtual IPipelineCache& getPipelineCache() = 0;
		virtual IShaderModuleCache& getShaderModuleCache() = 0;
		virtual IResourceSetLayoutCache& getResourceSetLayoutCache() = 0;
		virtual IPipelineLayoutCache& getPipelineLayoutCache() = 0;
		virtual IResourceSetManager& getResourceSetManager() = 0;
	};
}
