#pragma once
#include "VulkanPipelineCache.hpp"
#include "VulkanPipelineLayoutCache.hpp"
#include "VulkanRenderResourceManager.hpp"
#include "VulkanResourceSetLayoutCache.hpp"
#include "VulkanResourceSetManager.hpp"
#include "VulkanShaderModuleCache.hpp"

#include "engine/rendering/IRenderDevice.hpp"

namespace spite
{
	struct VulkanRenderContext;

	class VulkanRenderDevice : public IRenderDevice
	{
	private:
		VulkanRenderContext& m_context;
		vk::PipelineCache m_vkPipelineCache;
		VulkanRenderResourceManager m_resourceManager;
		VulkanPipelineCache m_pipelineCache;
		VulkanShaderModuleCache m_shaderModuleCache;
		VulkanResourceSetLayoutCache m_resourceSetLayoutCache;
		VulkanPipelineLayoutCache m_pipelineLayoutCache;
		VulkanResourceSetManager m_resourceSetManager;
		SamplerHandle m_defaultSampler;

	public:
		VulkanRenderDevice(VulkanRenderContext& context, const HeapAllocator& allocator);
		~VulkanRenderDevice() override;

		VulkanRenderDevice(const VulkanRenderDevice& other) = delete;
		VulkanRenderDevice(VulkanRenderDevice&& other) noexcept = delete;
		VulkanRenderDevice& operator=(const VulkanRenderDevice& other) = delete;
		VulkanRenderDevice& operator=(VulkanRenderDevice&& other) noexcept = delete;

		BufferHandle createBuffer(const BufferDesc& desc) override;
		void destroyBuffer(BufferHandle handle) override;

		TextureHandle createTexture(const TextureDesc& desc) override;
		void destroyTexture(TextureHandle handle) override;

		ImageViewHandle createImageView(const ImageViewDesc& desc) override;
		void destroyImageView(ImageViewHandle handle) override;

		SamplerHandle createSampler(const SamplerDesc& desc) override;
		void destroySampler(SamplerHandle handle) override;

		PipelineHandle getOrCreatePipeline(const PipelineDescription& description) override;
		ShaderModuleHandle getOrCreateShaderModule(const ShaderDescription& description) override;
		ResourceSetLayoutHandle getOrCreateResourceSetLayout(const ResourceSetLayoutDescription& description) override;
		PipelineLayoutHandle getOrCreatePipelineLayout(const PipelineLayoutDescription& description) override;

		VulkanRenderContext& getContext() { return m_context; }
		const VulkanRenderContext& getContext() const { return m_context; }
		vk::PipelineCache getVkPipelineCache() const { return m_vkPipelineCache; }
		IRenderResourceManager& getResourceManager() override { return m_resourceManager; }
		IPipelineCache& getPipelineCache() override { return m_pipelineCache; }
		IShaderModuleCache& getShaderModuleCache() override { return m_shaderModuleCache; }
		IResourceSetLayoutCache& getResourceSetLayoutCache() override { return m_resourceSetLayoutCache; }
		IPipelineLayoutCache& getPipelineLayoutCache() override { return m_pipelineLayoutCache; }
		IResourceSetManager& getResourceSetManager() override { return m_resourceSetManager; }
		SamplerHandle getDefaultSampler() const { return m_defaultSampler; }


		void clearSwapchainDependentCaches();
	};
}
