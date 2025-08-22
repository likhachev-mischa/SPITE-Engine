#pragma once
#include <queue>

#include "engine/rendering/IRenderResourceManager.hpp"
#include "engine/rendering/ResourcePool.hpp"
#include "base/CollectionAliases.hpp"
#include "base/VmaUsage.hpp"

namespace spite
{
	class VulkanBuffer;
	class VulkanImage;
	class VulkanImageView;
	class VulkanSampler;
	struct VulkanRenderContext;

	class VulkanRenderResourceManager : public IRenderResourceManager
	{
		friend class VulkanFramebufferCache;
		friend class VulkanRenderCommandBuffer;
		friend class VulkanSecondaryRenderCommandBuffer;
		friend class VulkanResourceSetManager;

	private:
		VulkanRenderContext& m_context;
		HeapAllocator m_allocator;
		ResourcePool<VulkanBuffer> m_buffers;
		heap_vector<BufferDesc> m_bufferDescs;
		ResourcePool<VulkanImage> m_textures;
		heap_vector<TextureDesc> m_textureDescs;
		ResourcePool<VulkanImageView> m_imageViews;
		ResourcePool<VulkanSampler> m_samplers;

		// --- Asynchronous Transfer Members ---
		static constexpr u32 MAX_CONCURRENT_TRANSFERS = 16;

		vk::CommandPool m_transferCommandPool;

		// Ring Buffer
		BufferHandle m_stagingRingBuffer;
		void* m_mappedStagingRingBuffer = nullptr; // Keep it persistently mapped
		sizet m_stagingRingBufferSize = 16 * MB; // 16MB
		sizet m_currentStagingOffset = 0;

		// Synchronization
		struct InFlightTransfer
		{
			vk::Fence fence;
			vk::CommandBuffer commandBuffer;
			sizet size; // Size of the allocation in the ring buffer
		};
		std::queue<InFlightTransfer> m_inFlightTransfers;
		heap_vector<vk::Fence> m_availableFences;
		heap_vector<vk::CommandBuffer> m_availableTransferCommandBuffers;


		template <typename T_Resource, typename... Args>
		GpuResourceHandle createResource(ResourcePool<T_Resource>& pool, VmaAllocator allocator, Args&&... args);

		template <typename T_Resource, typename... Args>
		GpuResourceHandle createResource(ResourcePool<T_Resource>& pool, vk::Device device, Args&&... args);

		template <typename T_Resource, typename... Args>
		GpuResourceHandle createResource(ResourcePool<T_Resource>& pool, Args&&... args);

		template <typename T_Resource>
		void destroyResource(ResourcePool<T_Resource>& pool, GpuResourceHandle handle);

		void recordCopy(BufferHandle src, BufferHandle dst, sizet size, sizet srcOffset, sizet dstOffset,
		                vk::CommandBuffer commandBuffer);

		VulkanBuffer& getBufferInternal(BufferHandle handle);
		const VulkanBuffer& getBufferInternal(BufferHandle handle) const;
		VulkanImage& getTexture(TextureHandle handle);
		const VulkanImage& getTexture(TextureHandle handle) const;
		VulkanImageView& getImageViewInternal(ImageViewHandle handle);
		const VulkanImageView& getImageViewInternal(ImageViewHandle handle) const;
		VulkanSampler& getSamplerInternal(SamplerHandle handle);
		const VulkanSampler& getSamplerInternal(SamplerHandle handle) const;

	public:
		VulkanRenderResourceManager(VulkanRenderContext& context, const HeapAllocator& allocator);
		~VulkanRenderResourceManager() override;

		VulkanRenderResourceManager(const VulkanRenderResourceManager&) = delete;
		VulkanRenderResourceManager& operator=(const VulkanRenderResourceManager&) = delete;
		VulkanRenderResourceManager(VulkanRenderResourceManager&&) = delete;
		VulkanRenderResourceManager& operator=(VulkanRenderResourceManager&&) = delete;

		BufferHandle createBuffer(const BufferDesc& desc) override;
		void destroyBuffer(BufferHandle handle) override;
		void* mapBuffer(BufferHandle handle) override;
		void unmapBuffer(BufferHandle handle) override;
		void updateBuffer(BufferHandle handle, const void* data, sizet size, sizet offset = 0) override;
		IBuffer& getBuffer(BufferHandle handle) override;
		const IBuffer& getBuffer(BufferHandle handle) const override;
		const BufferDesc& getBufferDesc(BufferHandle handle) const override;

		TextureHandle registerExternalTexture(vk::Image image, const TextureDesc& desc);

		TextureHandle createTexture(const TextureDesc& desc) override;
		void destroyTexture(TextureHandle handle) override;
		const TextureDesc& getTextureDesc(TextureHandle handle) const;

		ImageViewHandle createImageView(const ImageViewDesc& desc) override;
		ImageViewHandle registerExternalImageView(vk::ImageView imageView);
		void destroyImageView(ImageViewHandle handle) override;
		IImageView& getImageView(ImageViewHandle hanlde) override;
		const IImageView& getImageView(ImageViewHandle hanlde) const override;

		SamplerHandle createSampler(const SamplerDesc& desc) override;
		void destroySampler(SamplerHandle handle) override;
		ISampler& getSampler(SamplerHandle handle) override;
		const ISampler& getSampler(SamplerHandle handle) const override;

		void pollCompletedTransfers();
	};

	template <typename T_Resource, typename... Args>
	GpuResourceHandle VulkanRenderResourceManager::createResource(ResourcePool<T_Resource>& pool,
	                                                              VmaAllocator allocator, Args&&... args)
	{
		u32 index;
		if (!pool.freeIndices.empty())
		{
			index = pool.freeIndices.back();
			pool.freeIndices.pop_back();
			new(&pool.resources[index]) T_Resource(allocator, std::forward<Args>(args)...);
		}
		else
		{
			index = static_cast<u32>(pool.resources.size());
			pool.resources.emplace_back(allocator, std::forward<Args>(args)...);
			pool.generations.push_back(0);
		}
		return {index, pool.generations[index]};
	}

	template <typename T_Resource, typename... Args>
	GpuResourceHandle VulkanRenderResourceManager::createResource(ResourcePool<T_Resource>& pool, vk::Device device,
	                                                              Args&&... args)
	{
		u32 index;
		if (!pool.freeIndices.empty())
		{
			index = pool.freeIndices.back();
			pool.freeIndices.pop_back();
			new(&pool.resources[index]) T_Resource(device, std::forward<Args>(args)...);
		}
		else
		{
			index = static_cast<u32>(pool.resources.size());
			pool.resources.emplace_back(device, std::forward<Args>(args)...);
			pool.generations.push_back(0);
		}
		return {index, pool.generations[index]};
	}

	template <typename T_Resource, typename... Args>
	GpuResourceHandle VulkanRenderResourceManager::createResource(ResourcePool<T_Resource>& pool, Args&&... args)
	{
		u32 index;
		if (!pool.freeIndices.empty())
		{
			index = pool.freeIndices.back();
			pool.freeIndices.pop_back();
			new(&pool.resources[index]) T_Resource(std::forward<Args>(args)...);
		}
		else
		{
			index = static_cast<u32>(pool.resources.size());
			pool.resources.emplace_back(std::forward<Args>(args)...);
			pool.generations.push_back(0);
		}
		return {index, pool.generations[index]};
	}


	template <typename T_Resource>
	void VulkanRenderResourceManager::destroyResource(ResourcePool<T_Resource>& pool, GpuResourceHandle handle)
	{
		if (!handle.isValid() || handle.id >= pool.resources.size() || pool.generations[handle.id] != handle.generation)
		{
			SASSERTM(false, "Attempted to destroy a resource with an invalid or stale handle.")
			return;
		}

		// Explicitly call the destructor of the object being replaced.
		pool.resources[handle.id].~T_Resource();
		// Use placement-new to construct a default object in the same memory location.
		// This prevents the vector's destructor from calling the destructor on the same resource again.
		new(&pool.resources[handle.id]) T_Resource();

		pool.freeIndices.push_back(handle.id);
		++pool.generations[handle.id];
	}
}
