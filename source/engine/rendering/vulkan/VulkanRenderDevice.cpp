#include "VulkanRenderDevice.hpp"
#include "VulkanRenderContext.hpp"
#include "base/File.hpp"
#include "base/Logging.hpp"

namespace spite
{
	constexpr const char* PIPELINE_CACHE_FILENAME = "pipeline_cache.bin";

	VulkanRenderDevice::VulkanRenderDevice(VulkanRenderContext& context, const HeapAllocator& allocator)
		: m_context(context),
		  m_resourceManager(context, allocator),
		  m_pipelineCache(*this, allocator),
		  m_shaderModuleCache(context, allocator),
		  m_resourceSetLayoutCache(context, allocator),
		  m_pipelineLayoutCache(*this, allocator),
		  m_resourceSetManager(*this, allocator)
	{
		// --- Load IPipeline Cache ---
		std::vector<char> cacheData = readBinaryFile(PIPELINE_CACHE_FILENAME);
		vk::PipelineCacheCreateInfo cacheInfo{};
		if (!cacheData.empty())
		{
			SDEBUG_LOG("Found pipeline cache file '%s', loading.\n", PIPELINE_CACHE_FILENAME)
			cacheInfo.initialDataSize = cacheData.size();
			cacheInfo.pInitialData = cacheData.data();
		}

		auto [result, pipelineCache] = m_context.device.createPipelineCache(cacheInfo);
		SASSERT_VULKAN(result)
		m_vkPipelineCache = pipelineCache;

		SamplerDesc defaultSamplerDesc{};
		m_defaultSampler = m_resourceManager.createSampler(defaultSamplerDesc);
	}

	VulkanRenderDevice::~VulkanRenderDevice()
	{
		// --- Save IPipeline Cache ---
		auto [result, data] = m_context.device.getPipelineCacheData(m_vkPipelineCache);
		if (result == vk::Result::eSuccess && !data.empty())
		{
			SDEBUG_LOG("Saving pipeline cache to '%s'.\n", PIPELINE_CACHE_FILENAME)
			writeBinaryFile(PIPELINE_CACHE_FILENAME, data.data(),data.size());
		}

		m_context.device.destroyPipelineCache(m_vkPipelineCache);
	}

	BufferHandle VulkanRenderDevice::createBuffer(const BufferDesc& desc)
	{
		return m_resourceManager.createBuffer(desc);
	}

	void VulkanRenderDevice::destroyBuffer(BufferHandle handle)
	{
		m_resourceManager.destroyBuffer(handle);
	}

	TextureHandle VulkanRenderDevice::createTexture(const TextureDesc& desc)
	{
		return m_resourceManager.createTexture(desc);
	}

	void VulkanRenderDevice::destroyTexture(TextureHandle handle)
	{
		m_resourceManager.destroyTexture(handle);
	}

	ImageViewHandle VulkanRenderDevice::createImageView(const ImageViewDesc& desc)
	{
		return m_resourceManager.createImageView(desc);
	}

	void VulkanRenderDevice::destroyImageView(ImageViewHandle handle)
	{
		m_resourceManager.destroyImageView(handle);
	}

	SamplerHandle VulkanRenderDevice::createSampler(const SamplerDesc& desc)
	{
		return m_resourceManager.createSampler(desc);
	}

	void VulkanRenderDevice::destroySampler(SamplerHandle handle)
	{
		m_resourceManager.destroySampler(handle);
	}

	PipelineHandle VulkanRenderDevice::getOrCreatePipeline(const PipelineDescription& description)
	{
		return m_pipelineCache.getOrCreatePipeline(description);
	}

	ShaderModuleHandle VulkanRenderDevice::getOrCreateShaderModule(const ShaderDescription& description)
	{
		return m_shaderModuleCache.getOrCreateShaderModule(description);
	}

	ResourceSetLayoutHandle VulkanRenderDevice::getOrCreateResourceSetLayout(
		const ResourceSetLayoutDescription& description)
	{
		return m_resourceSetLayoutCache.getOrCreateLayout(description);
	}

	PipelineLayoutHandle VulkanRenderDevice::getOrCreatePipelineLayout(const PipelineLayoutDescription& description)
	{
		return m_pipelineLayoutCache.getOrCreatePipelineLayout(description);
	}

	void VulkanRenderDevice::clearSwapchainDependentCaches()
	{
		m_pipelineCache.clear();
	}
}
