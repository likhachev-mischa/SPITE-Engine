#include "VulkanRenderResourceManager.hpp"

#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"
#include "VulkanImageView.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanSampler.hpp"
#include "VulkanTypeMappings.hpp"

#include "base/Assert.hpp"


namespace spite
{
	VulkanRenderResourceManager::VulkanRenderResourceManager(VulkanRenderContext& context,
	                                                         const HeapAllocator& allocator)
		: m_context(context),
		  m_allocator(allocator),
		  m_buffers(allocator),
		  m_bufferDescs(makeHeapVector<BufferDesc>(allocator)),
		  m_textures(allocator),
		  m_textureDescs(makeHeapVector<TextureDesc>(allocator)),
		  m_imageViews(allocator),
		  m_samplers(allocator),
		  m_availableFences(makeHeapVector<vk::Fence>(allocator)),
		  m_availableTransferCommandBuffers(makeHeapVector<vk::CommandBuffer>(allocator))
	{
		// --- Create Transfer Command Pool ---
		vk::CommandPoolCreateInfo poolInfo{};
		poolInfo.queueFamilyIndex = m_context.transferQueueFamily;
		// Command buffers are now managed individually, and we will reset them implicitly via vkBeginCommandBuffer
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

		auto cpres = m_context.device.createCommandPool(poolInfo);
		SASSERT_VULKAN(cpres.result)
		m_transferCommandPool = cpres.value;

		// --- Create Command Buffer and Fence Pools ---
		m_availableFences.reserve(MAX_CONCURRENT_TRANSFERS);
		m_availableTransferCommandBuffers.reserve(MAX_CONCURRENT_TRANSFERS);

		vk::CommandBufferAllocateInfo cbAllocInfo{};
		cbAllocInfo.level = vk::CommandBufferLevel::ePrimary;
		cbAllocInfo.commandPool = m_transferCommandPool;
		cbAllocInfo.commandBufferCount = MAX_CONCURRENT_TRANSFERS;
		auto [cbRes, cbuffers] = m_context.device.allocateCommandBuffers(cbAllocInfo);
		SASSERT_VULKAN(cbRes)

		vk::FenceCreateInfo fenceInfo{}; // Fences are created in an unsignaled state
		for (u32 i = 0; i < MAX_CONCURRENT_TRANSFERS; ++i)
		{
			auto [fenceRes, fence] = m_context.device.createFence(fenceInfo);
			SASSERT_VULKAN(fenceRes)
			m_availableFences.push_back(fence);
			m_availableTransferCommandBuffers.push_back(cbuffers[i]);
		}

		// --- Create and map the staging ring buffer ---
		BufferDesc stagingBufferDesc;
		stagingBufferDesc.size = m_stagingRingBufferSize;
		stagingBufferDesc.usage = BufferUsage::TRANSFER_SRC;
		stagingBufferDesc.memoryUsage = MemoryUsage::CPU_ONLY; // CPU write, GPU read
		m_stagingRingBuffer = VulkanRenderResourceManager::createBuffer(stagingBufferDesc);
		m_mappedStagingRingBuffer = VulkanRenderResourceManager::mapBuffer(m_stagingRingBuffer);
	}

	VulkanRenderResourceManager::~VulkanRenderResourceManager()
	{
		// Unmap and destroy the ring buffer
		VulkanRenderResourceManager::unmapBuffer(m_stagingRingBuffer);
		VulkanRenderResourceManager::destroyBuffer(m_stagingRingBuffer);

		// Destroy all fences
		for (vk::Fence fence : m_availableFences)
		{
			m_context.device.destroyFence(fence);
		}
		while (!m_inFlightTransfers.empty())
		{
			m_context.device.destroyFence(m_inFlightTransfers.front().fence);
			m_inFlightTransfers.pop();
		}

		m_context.device.destroyCommandPool(m_transferCommandPool);
		// The resource vectors will automatically call destructors for all contained objects.
	}

	BufferHandle VulkanRenderResourceManager::createBuffer(const BufferDesc& desc)
	{
		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.size = desc.size;
		bufferInfo.usage = vulkan::to_vulkan_buffer_usage(desc.usage);

		// If the buffer can be used in a descriptor, it needs the device address flag.
		if (has_flag(desc.usage, BufferUsage::UNIFORM_BUFFER) || has_flag(desc.usage, BufferUsage::STORAGE_BUFFER))
		{
			bufferInfo.usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
		}

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = vulkan::to_vma_memory_usage(desc.memoryUsage);

		u32 index;
		if (!m_buffers.freeIndices.empty())
		{
			index = m_buffers.freeIndices.back();
			m_buffers.freeIndices.pop_back();
			new(&m_buffers.resources[index]) VulkanBuffer(m_context.device, m_context.allocator, bufferInfo, allocInfo);
		}
		else
		{
			index = static_cast<u32>(m_buffers.resources.size());
			m_buffers.resources.emplace_back(m_context.device, m_context.allocator, bufferInfo, allocInfo);
			m_buffers.generations.push_back(0);
		}

		if (index >= m_bufferDescs.size())
		{
			m_bufferDescs.resize(index + 1);
		}
		m_bufferDescs[index] = desc;

		return { index, m_buffers.generations[index] };
	}

	void VulkanRenderResourceManager::destroyBuffer(BufferHandle handle)
	{
		destroyResource(m_buffers, handle);
	}

	void* VulkanRenderResourceManager::mapBuffer(BufferHandle handle)
	{
		VulkanBuffer& buffer = getBufferInternal(handle);
		void* pData;
		vmaMapMemory(m_context.allocator, buffer.getAllocation(), &pData);
		return pData;
	}

	void VulkanRenderResourceManager::unmapBuffer(BufferHandle handle)
	{
		VulkanBuffer& buffer = getBufferInternal(handle);
		vmaUnmapMemory(m_context.allocator, buffer.getAllocation());
	}

	void VulkanRenderResourceManager::updateBuffer(BufferHandle handle, const void* data, sizet size, sizet offset)
	{
		VulkanBuffer& buffer = getBufferInternal(handle);

		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(m_context.allocator, buffer.getAllocation(), &allocInfo);

		// If the buffer is mappable, just map and copy. This is the fastest path.
		if (allocInfo.pMappedData)
		{
			char* pData = static_cast<char*>(allocInfo.pMappedData);
			memcpy(pData + offset, data, size);
			return;
		}

		// --- Staged Transfer for GPU-only buffers ---
		pollCompletedTransfers();

		SASSERTM(size <= m_stagingRingBufferSize, "Update size is larger than the entire staging buffer.")
		SASSERTM(!m_availableFences.empty(), "Ran out of fences for async transfers. Increase pool size or wait.")
		SASSERTM(!m_availableTransferCommandBuffers.empty(),
		         "Ran out of command buffers for async transfers. Increase pool size or wait.")

		if (m_currentStagingOffset + size > m_stagingRingBufferSize)
		{
			m_currentStagingOffset = 0;
		}
		sizet currentAllocationOffset = m_currentStagingOffset;
		m_currentStagingOffset += size;

		vk::Fence transferFence = m_availableFences.back();
		m_availableFences.pop_back();

		vk::CommandBuffer transferCommandBuffer = m_availableTransferCommandBuffers.back();
		m_availableTransferCommandBuffers.pop_back();

		memcpy(static_cast<char*>(m_mappedStagingRingBuffer) + currentAllocationOffset, data, size);

		// Record commands and submit
		recordCopy(m_stagingRingBuffer, handle, size, currentAllocationOffset, offset, transferCommandBuffer);

		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &transferCommandBuffer;

		auto res = m_context.transferQueue.submit(1, &submitInfo, transferFence);
		SASSERT_VULKAN(res)

		m_inFlightTransfers.push({transferFence, transferCommandBuffer, size});
	}

	IBuffer& VulkanRenderResourceManager::getBuffer(BufferHandle handle)
	{
		return getBufferInternal(handle);
	}

	const IBuffer& VulkanRenderResourceManager::getBuffer(BufferHandle handle) const
	{
		return getBufferInternal(handle);
	}

	const BufferDesc& VulkanRenderResourceManager::getBufferDesc(BufferHandle handle) const
	{
		SASSERT(handle.isValid() && handle.id < m_bufferDescs.size())
		return m_bufferDescs[handle.id];
	}

	void VulkanRenderResourceManager::recordCopy(BufferHandle srcHandle, BufferHandle dstHandle, sizet size,
	                                             sizet srcOffset, sizet dstOffset, vk::CommandBuffer commandBuffer)
	{
		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		auto res = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(res)

		VulkanBuffer& srcBuffer = getBufferInternal(srcHandle);
		VulkanBuffer& dstBuffer = getBufferInternal(dstHandle);

		vk::BufferCopy copyRegion;
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;
		commandBuffer.copyBuffer(srcBuffer.get(), dstBuffer.get(), 1, &copyRegion);

		res = commandBuffer.end();
		SASSERT_VULKAN(res)
	}

	void VulkanRenderResourceManager::pollCompletedTransfers()
	{
		while (!m_inFlightTransfers.empty())
		{
			InFlightTransfer& transfer = m_inFlightTransfers.front();

			vk::Result fenceStatus = m_context.device.getFenceStatus(transfer.fence);
			if (fenceStatus == vk::Result::eNotReady)
			{
				break;
			}
			SASSERT_VULKAN(fenceStatus)

			auto res = m_context.device.resetFences(1, &transfer.fence);
			SASSERT_VULKAN(res)
			m_availableFences.push_back(transfer.fence);
			m_availableTransferCommandBuffers.push_back(transfer.commandBuffer);

			m_inFlightTransfers.pop();
		}
	}

	TextureHandle VulkanRenderResourceManager::createTexture(const TextureDesc& desc)
	{
		vk::ImageCreateInfo imageInfo{};
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.extent.width = desc.width;
		imageInfo.extent.height = desc.height;
		imageInfo.extent.depth = desc.depth;
		imageInfo.mipLevels = desc.mipLevels;
		imageInfo.arrayLayers = desc.arrayLayers;
		imageInfo.format = vulkan::to_vulkan_format(desc.format);
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.usage = vulkan::to_vulkan_texture_usage(desc.usage);
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.sharingMode = vk::SharingMode::eExclusive;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = vulkan::to_vma_memory_usage(desc.memoryUsage);

		GpuResourceHandle handle = createResource(m_textures, m_context.allocator, imageInfo, allocInfo);

		if (handle.id >= m_textureDescs.size())
		{
			m_textureDescs.resize(handle.id + 1);
		}
		m_textureDescs[handle.id] = desc;

		return static_cast<TextureHandle>(handle);
	}

	void VulkanRenderResourceManager::destroyTexture(TextureHandle handle)
	{
		destroyResource(m_textures, handle);
	}

	TextureHandle VulkanRenderResourceManager::registerExternalTexture(vk::Image image, const TextureDesc& desc)
	{
		GpuResourceHandle handle = createResource(m_textures, image);
		if (handle.id >= m_textureDescs.size())
		{
			m_textureDescs.resize(handle.id + 1);
		}
		m_textureDescs[handle.id] = desc;

		return static_cast<TextureHandle>(handle);
	}

	const TextureDesc& VulkanRenderResourceManager::getTextureDesc(TextureHandle handle) const
	{
		SASSERT(handle.isValid() && handle.id < m_textureDescs.size())
		return m_textureDescs[handle.id];
	}

	ImageViewHandle VulkanRenderResourceManager::createImageView(const ImageViewDesc& desc)
	{
		VulkanImage& image = getTexture(desc.texture);
		vk::ImageAspectFlags aspectFlags = vulkan::to_vulkan_aspect_mask(desc.format);

		GpuResourceHandle handle = createResource(m_imageViews, m_context.device, image.get(),
		                                          vulkan::to_vulkan_format(desc.format), aspectFlags);
		return static_cast<ImageViewHandle>(handle);
	}

	ImageViewHandle VulkanRenderResourceManager::registerExternalImageView(vk::ImageView imageView)
	{
		return static_cast<ImageViewHandle>(createResource(m_imageViews, imageView));
	}

	void VulkanRenderResourceManager::destroyImageView(ImageViewHandle handle)
	{
		destroyResource(m_imageViews, handle);
	}

	IImageView& VulkanRenderResourceManager::getImageView(ImageViewHandle hanlde)
	{
		return getImageViewInternal(hanlde);
	}

	const IImageView& VulkanRenderResourceManager::getImageView(ImageViewHandle hanlde) const
	{
		return getImageViewInternal(hanlde);
	}

	SamplerHandle VulkanRenderResourceManager::createSampler(const SamplerDesc& desc)
	{
		vk::SamplerCreateInfo samplerInfo{};
		samplerInfo.magFilter = vulkan::to_vulkan_filter(desc.magFilter);
		samplerInfo.minFilter = vulkan::to_vulkan_filter(desc.minFilter);
		samplerInfo.addressModeU = vulkan::to_vulkan_address_mode(desc.addressModeU);
		samplerInfo.addressModeV = vulkan::to_vulkan_address_mode(desc.addressModeV);
		samplerInfo.addressModeW = vulkan::to_vulkan_address_mode(desc.addressModeW);
		samplerInfo.anisotropyEnable = desc.anisotropyEnable;
		samplerInfo.maxAnisotropy = desc.maxAnisotropy;
		samplerInfo.borderColor = vulkan::to_vulkan_border_color(desc.borderColor);
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = desc.compareEnable;
		samplerInfo.compareOp = vulkan::to_vulkan_compare_op(desc.compareOp);
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		GpuResourceHandle handle = createResource(m_samplers, m_context.device, samplerInfo);
		return static_cast<SamplerHandle>(handle);
	}

	void VulkanRenderResourceManager::destroySampler(SamplerHandle handle)
	{
		destroyResource(m_samplers, handle);
	}

	VulkanBuffer& VulkanRenderResourceManager::getBufferInternal(BufferHandle handle)
	{
		SASSERT(
			handle.isValid() && handle.id < m_buffers.resources.size() && m_buffers.generations[handle.id] == handle.
			generation)
		return m_buffers.resources[handle.id];
	}

	const VulkanBuffer& VulkanRenderResourceManager::getBufferInternal(BufferHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_buffers.resources.size() && m_buffers.generations[handle.id] == handle.
			generation)
		return m_buffers.resources[handle.id];
	}

	VulkanImage& VulkanRenderResourceManager::getTexture(TextureHandle handle)
	{
		SASSERT(
			handle.isValid() && handle.id < m_textures.resources.size() && m_textures.generations[handle.id] == handle.
			generation)
		return m_textures.resources[handle.id];
	}

	const VulkanImage& VulkanRenderResourceManager::getTexture(TextureHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_textures.resources.size() && m_textures.generations[handle.id] == handle.
			generation)
		return m_textures.resources[handle.id];
	}

	VulkanImageView& VulkanRenderResourceManager::getImageViewInternal(ImageViewHandle handle)
	{
		SASSERT(
			handle.isValid() && handle.id < m_imageViews.resources.size() && m_imageViews.generations[handle.id] ==
			handle.generation)
		return m_imageViews.resources[handle.id];
	}

	const VulkanImageView& VulkanRenderResourceManager::getImageViewInternal(ImageViewHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_imageViews.resources.size() && m_imageViews.generations[handle.id] ==
			handle.generation)
		return m_imageViews.resources[handle.id];
	}

	VulkanSampler& VulkanRenderResourceManager::getSamplerInternal(SamplerHandle handle)
	{
		SASSERT(
			handle.isValid() && handle.id < m_samplers.resources.size() && m_samplers.generations[handle.id] == handle.
			generation)
		return m_samplers.resources[handle.id];
	}

	const VulkanSampler& VulkanRenderResourceManager::getSamplerInternal(SamplerHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_samplers.resources.size() && m_samplers.generations[handle.id] == handle.
			generation)
		return m_samplers.resources[handle.id];
	}

	ISampler& VulkanRenderResourceManager::getSampler(SamplerHandle handle)
	{
		return getSamplerInternal(handle);
	}

	const ISampler& VulkanRenderResourceManager::getSampler(SamplerHandle handle) const
	{
		return getSamplerInternal(handle);
	}
}
