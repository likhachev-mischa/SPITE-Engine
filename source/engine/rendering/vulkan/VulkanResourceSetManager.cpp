#include "VulkanResourceSetManager.hpp"

#include <memory>

#include <EASTL/array.h>

#include "VulkanBuffer.hpp"
#include "VulkanDescriptorSetLayout.hpp"
#include "VulkanImageView.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanRenderResourceManager.hpp"
#include "VulkanResourceSetLayoutCache.hpp"
#include "VulkanSampler.hpp"
#include "VulkanTypeMappings.hpp"

#include "base/Assert.hpp"
#include "base/CollectionUtilities.hpp"

namespace spite
{
#if !defined(SPITE_USE_DESCRIPTOR_SETS)
	constexpr sizet RESOURCE_DESCRIPTOR_BUFFER_SIZE = 16 * MB;
	constexpr sizet SAMPLER_DESCRIPTOR_BUFFER_SIZE = 1 * MB;

	// Helper to align offsets
	static vk::DeviceSize alignUp(vk::DeviceSize size, vk::DeviceSize alignment)
	{
		return (size + alignment - 1) & ~(alignment - 1);
	}
#endif

	VulkanResourceSetManager::VulkanResourceSetManager(VulkanRenderDevice& device, const HeapAllocator& allocator)
		: m_device(device),
		  m_allocator(allocator),
		  m_sets(allocator)
	{
		auto& renderContext = m_device.getContext();

#if defined(SPITE_USE_DESCRIPTOR_SETS)
		// Create a single, large descriptor pool for simplicity.
		// A more robust implementation might use multiple pools or a pool manager.
		eastl::array<vk::DescriptorPoolSize, 5> poolSizes = {
			{
				{vk::DescriptorType::eUniformBuffer, 1000},
				{vk::DescriptorType::eStorageBuffer, 1000},
				{vk::DescriptorType::eCombinedImageSampler, 1000},
				{vk::DescriptorType::eStorageImage, 1000},
				{vk::DescriptorType::eSampler, 1000}
			}
		};
		vk::DescriptorPoolCreateInfo poolInfo;
		poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
		poolInfo.maxSets = 5000; // Max number of sets that can be allocated
		poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		auto [result, pool] = renderContext.device.createDescriptorPool(poolInfo);
		SASSERT_VULKAN(result)
		m_descriptorPool = pool;
#else
		// Get descriptor buffer properties
		vk::PhysicalDeviceProperties2 props2;
		m_descriptorBufferProperties.pNext = nullptr;
		props2.pNext = &m_descriptorBufferProperties;
		renderContext.physicalDevice.getProperties2(&props2);

		// Create resource descriptor buffer
		BufferDesc resourceDesc;
		resourceDesc.size = RESOURCE_DESCRIPTOR_BUFFER_SIZE;
		resourceDesc.usage = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE_BUFFER |
			BufferUsage::RESOURCE_DESCRIPTOR_BUFFER;
		resourceDesc.memoryUsage = MemoryUsage::CPU_TO_GPU;
		m_resourceDescriptorBuffer = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager()).
			createBuffer(resourceDesc);
		m_resourceDescriptorBufferPtr = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager()).
			mapBuffer(m_resourceDescriptorBuffer);

		// Create sampler descriptor buffer
		BufferDesc samplerDesc;
		samplerDesc.size = SAMPLER_DESCRIPTOR_BUFFER_SIZE;
		samplerDesc.usage = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE_BUFFER |
			BufferUsage::SAMPLER_DESCRIPTOR_BUFFER;
		samplerDesc.memoryUsage = MemoryUsage::CPU_TO_GPU;
		m_samplerDescriptorBuffer = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager()).
			createBuffer(samplerDesc);
		m_samplerDescriptorBufferPtr = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager()).
			mapBuffer(m_samplerDescriptorBuffer);
#endif
	}

	VulkanResourceSetManager::~VulkanResourceSetManager()
	{
#if defined(SPITE_USE_DESCRIPTOR_SETS)
		if (m_descriptorPool)
		{
			m_device.getContext().device.destroyDescriptorPool(m_descriptorPool);
		}
#else
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager());
		resourceManager.unmapBuffer(m_resourceDescriptorBuffer);
		resourceManager.destroyBuffer(m_resourceDescriptorBuffer);
		resourceManager.unmapBuffer(m_samplerDescriptorBuffer);
		resourceManager.destroyBuffer(m_samplerDescriptorBuffer);
#endif
	}

	ResourceSetHandle VulkanResourceSetManager::allocateSet(ResourceSetLayoutHandle layoutHandle)
	{
		auto& layoutCache = m_device.getResourceSetLayoutCache();
		auto& vkLayoutCache = static_cast<VulkanResourceSetLayoutCache&>(layoutCache);
		auto& layout = static_cast<VulkanDescriptorSetLayout&>(vkLayoutCache.getLayout(layoutHandle));
		auto& renderContext = m_device.getContext();

		VulkanResourceSet newSetAllocation;
		newSetAllocation.layoutHandle = layoutHandle;

#if defined(SPITE_USE_DESCRIPTOR_SETS)
		vk::DescriptorSetAllocateInfo allocInfo;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = 1;
		auto vkLayout = layout.get();
		allocInfo.pSetLayouts = &vkLayout;

		auto [result, sets] = renderContext.device.allocateDescriptorSets(allocInfo);
		SASSERT_VULKAN(result)
		newSetAllocation.descriptorSet = sets[0];
#else
		vk::DeviceSize layoutSize;
		renderContext.pfnGetDescriptorSetLayoutSizeEXT(renderContext.device, layout.get(), &layoutSize);

		const auto& layoutDesc = layoutCache.getLayoutDescription(layoutHandle);
		SASSERT(!layoutDesc.bindings.empty());
		DescriptorType firstBindingType = layoutDesc.bindings[0].type;

		newSetAllocation.size = layoutSize;

		if (firstBindingType == DescriptorType::SAMPLER)
		{
			SASSERT(m_samplerDescriptorBufferOffset + layoutSize <= SAMPLER_DESCRIPTOR_BUFFER_SIZE);
			newSetAllocation.buffer = m_samplerDescriptorBuffer;
			newSetAllocation.offset = m_samplerDescriptorBufferOffset;
			m_samplerDescriptorBufferOffset += alignUp(
				layoutSize, m_descriptorBufferProperties.descriptorBufferOffsetAlignment);
		}
		else
		{
			SASSERT(m_resourceDescriptorBufferOffset + layoutSize <= RESOURCE_DESCRIPTOR_BUFFER_SIZE);
			newSetAllocation.buffer = m_resourceDescriptorBuffer;
			newSetAllocation.offset = m_resourceDescriptorBufferOffset;
			m_resourceDescriptorBufferOffset += alignUp(
				layoutSize, m_descriptorBufferProperties.descriptorBufferOffsetAlignment);
		}
#endif

		u32 index;
		if (!m_sets.freeIndices.empty())
		{
			index = m_sets.freeIndices.back();
			m_sets.freeIndices.pop_back();
			m_sets.resources[index] = newSetAllocation;
		}
		else
		{
			index = static_cast<u32>(m_sets.resources.size());
			m_sets.resources.push_back(newSetAllocation);
			m_sets.generations.push_back(0);
		}

		return {index, m_sets.generations[index]};
	}

	void VulkanResourceSetManager::updateSet(ResourceSetHandle setHandle, const sbo_vector<ResourceWrite>& writes)
	{
		if (writes.empty())
		{
			return;
		}

		auto& renderContext = m_device.getContext();
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_device.getResourceManager());
		const VulkanResourceSet& set = static_cast<const VulkanResourceSet&>(getSet(setHandle));

#if defined(SPITE_USE_DESCRIPTOR_SETS)
		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto descriptorWrites = makeScratchVector<vk::WriteDescriptorSet>(FrameScratchAllocator::get());
		// These need to live until vkUpdateDescriptorSets is called
		auto bufferInfos = makeScratchVector<vk::DescriptorBufferInfo>(FrameScratchAllocator::get());
		auto imageInfos = makeScratchVector<vk::DescriptorImageInfo>(FrameScratchAllocator::get());

		for (const auto& write : writes)
		{
			vk::WriteDescriptorSet descriptorWrite{};
			descriptorWrite.dstSet = set.descriptorSet;
			descriptorWrite.dstBinding = write.binding;
			descriptorWrite.dstArrayElement = write.arrayElement;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.descriptorType = vulkan::to_vulkan_descriptor_type(write.type);

			switch (write.type)
			{
			case DescriptorType::UNIFORM_BUFFER:
				{
					BufferHandle bufferHandle = static_cast<BufferHandle>(write.resource);
					VulkanBuffer& buffer = static_cast<VulkanBuffer&>(resourceManager.getBuffer(bufferHandle));
					const BufferDesc& desc = resourceManager.getBufferDesc(bufferHandle);
					bufferInfos.push_back({buffer.get(), 0, desc.size});
					descriptorWrite.pBufferInfo = &bufferInfos.back();
					break;
				}
			case DescriptorType::SAMPLED_TEXTURE:
				{
					VulkanImageView& view = static_cast<VulkanImageView&>(resourceManager.getImageView(
						static_cast<ImageViewHandle>(write.resource)));
					SamplerHandle defaultSamplerHandle = m_device.getDefaultSampler();
					VulkanSampler& sampler = static_cast<VulkanSampler&>(resourceManager.getSampler(
						defaultSamplerHandle));
					imageInfos.push_back({sampler.get(), view.get(), vk::ImageLayout::eShaderReadOnlyOptimal});
					descriptorWrite.pImageInfo = &imageInfos.back();
					break;
				}
			case DescriptorType::SAMPLER:
				{
					VulkanSampler& sampler = static_cast<VulkanSampler&>(resourceManager.getSampler(
						static_cast<SamplerHandle>(write.resource)));
					imageInfos.push_back({sampler.get(), nullptr, vk::ImageLayout::eUndefined});
					descriptorWrite.pImageInfo = &imageInfos.back();
					break;
				}
			default:
				SASSERTM(false, "Unsupported descriptor type for descriptor set write.")
				continue;
			}
			descriptorWrites.push_back(descriptorWrite);
		}
		renderContext.device.updateDescriptorSets(descriptorWrites, nullptr);

#else // Descriptor Buffers
		auto& layoutCache = m_device.getResourceSetLayoutCache();
		char* bufferPtr = static_cast<char*>(set.buffer == m_resourceDescriptorBuffer
			                                     ? m_resourceDescriptorBufferPtr
			                                     : m_samplerDescriptorBufferPtr);

		for (const auto& write : writes)
		{
			auto& vkLayoutCache = static_cast<VulkanResourceSetLayoutCache&>(layoutCache);
			auto& layout = static_cast<VulkanDescriptorSetLayout&>(vkLayoutCache.getLayout(set.layoutHandle));
			vk::DeviceSize bindingOffset;
			renderContext.pfnGetDescriptorSetLayoutBindingOffsetEXT(
				renderContext.device, layout.get(), write.binding, &bindingOffset);

			char* writePos = bufferPtr + set.offset + bindingOffset;

			VkDescriptorGetInfoEXT descriptorInfo{};
			descriptorInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
			descriptorInfo.pNext = nullptr;
			descriptorInfo.type = static_cast<VkDescriptorType>(vulkan::to_vulkan_descriptor_type(write.type));

			switch (write.type)
			{
			case DescriptorType::UNIFORM_BUFFER:
				{
					BufferHandle bufferHandle = static_cast<BufferHandle>(write.resource);
					VulkanBuffer& buffer = static_cast<VulkanBuffer&>(resourceManager.getBuffer(bufferHandle));
					const BufferDesc& desc = resourceManager.getBufferDesc(bufferHandle);

					VkDescriptorAddressInfoEXT addrInfo{};
					addrInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT;
					addrInfo.pNext = nullptr;
					addrInfo.address = buffer.getDeviceAddress();
					addrInfo.range = desc.size;
					addrInfo.format = VK_FORMAT_UNDEFINED;

					descriptorInfo.data.pUniformBuffer = &addrInfo;
					break;
				}
			case DescriptorType::SAMPLED_TEXTURE:
				{
					VulkanImageView& view = static_cast<VulkanImageView&>(resourceManager.getImageView(
						static_cast<ImageViewHandle>(write.resource)));
					SamplerHandle defaultSamplerHandle = m_device.getDefaultSampler();
					VulkanSampler& sampler = static_cast<VulkanSampler&>(resourceManager.getSampler(
						defaultSamplerHandle));

					VkDescriptorImageInfo imageInfo{
						sampler.get(), view.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					};

					descriptorInfo.data.pCombinedImageSampler = &imageInfo;
					break;
				}
			case DescriptorType::SAMPLER:
				{
					VulkanSampler& sampler = static_cast<VulkanSampler&>(resourceManager.getSampler(
						static_cast<SamplerHandle>(write.resource)));

					VkSampler vkSampler = sampler.get();
					descriptorInfo.data.pSampler = &vkSampler;
					break;
				}
			default:
				SASSERTM(false, "Unsupported descriptor type for descriptor buffer write.");
				continue;
			}

			renderContext.pfnGetDescriptorEXT(renderContext.device, &descriptorInfo, getDescriptorSize(write.type),
			                                  writePos);
		}
#endif
	}

#if !defined(SPITE_USE_DESCRIPTOR_SETS)
	BufferHandle VulkanResourceSetManager::getDescriptorBuffer(DescriptorType type) const
	{
		if (type == DescriptorType::SAMPLER)
		{
			return m_samplerDescriptorBuffer;
		}
		return m_resourceDescriptorBuffer;
	}

	sizet VulkanResourceSetManager::getDescriptorSize(DescriptorType type) const
	{
		switch (type)
		{
		case DescriptorType::UNIFORM_BUFFER:
			return m_descriptorBufferProperties.uniformBufferDescriptorSize;
		case DescriptorType::STORAGE_BUFFER:
			return m_descriptorBufferProperties.storageBufferDescriptorSize;
		case DescriptorType::SAMPLED_TEXTURE:
			return m_descriptorBufferProperties.combinedImageSamplerDescriptorSize;
		case DescriptorType::STORAGE_TEXTURE:
			return m_descriptorBufferProperties.storageImageDescriptorSize;
		case DescriptorType::SAMPLER:
			return m_descriptorBufferProperties.samplerDescriptorSize;
		default:
			SASSERT(false);
			return 0;
		}
	}
#endif

	IResourceSet& VulkanResourceSetManager::getSet(ResourceSetHandle handle)
	{
		SASSERT(
			handle.isValid() && handle.id < m_sets.resources.size() && m_sets.generations[handle.id]
			== handle.generation)
		return m_sets.resources[handle.id];
	}

	const IResourceSet& VulkanResourceSetManager::getSet(ResourceSetHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_sets.resources.size() && m_sets.generations[handle.id]
			== handle.generation)
		return m_sets.resources[handle.id];
	}
}
