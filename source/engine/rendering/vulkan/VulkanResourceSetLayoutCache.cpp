#include "VulkanResourceSetLayoutCache.hpp"
#include "base/Assert.hpp"
#include "base/CollectionUtilities.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanTypeMappings.hpp"
#include "VulkanDescriptorSetLayout.hpp"
#include <memory>

namespace spite
{
	VulkanResourceSetLayoutCache::VulkanResourceSetLayoutCache(VulkanRenderContext& context,
	                                                           const HeapAllocator& allocator)
		: m_context(context),
		  m_allocator(allocator),
		  m_cache(
			  makeHeapMap<ResourceSetLayoutDescription, ResourceSetLayoutHandle, ResourceSetLayoutDescription::Hash>(
				  allocator)),
		  m_layouts(allocator),
		  m_layoutDescriptions(makeHeapVector<ResourceSetLayoutDescription>(allocator))
	{
	}

	VulkanResourceSetLayoutCache::~VulkanResourceSetLayoutCache()
	{
	}

	ResourceSetLayoutHandle VulkanResourceSetLayoutCache::getOrCreateLayout(
		const ResourceSetLayoutDescription& description)
	{
		// --- Validation ---
		if (!description.bindings.empty())
		{
			bool hasSampler = false;
			bool hasNonSampler = false;
			for (const auto& binding : description.bindings)
			{
				if (binding.type == DescriptorType::SAMPLER)
				{
					hasSampler = true;
				}
				else
				{
					hasNonSampler = true;
				}
			}
			SASSERTM(!(hasSampler && hasNonSampler), "Descriptor set layout mixes sampler and non-sampler types. This is not supported.")
		}

		auto it = m_cache.find(description);
		if (it != m_cache.end())
		{
			return it->second;
		}

		auto& renderContext = m_context;

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto bindings = makeScratchVector<vk::DescriptorSetLayoutBinding>(FrameScratchAllocator::get());

		for (const auto& binding : description.bindings)
		{
			bindings.push_back(vk::DescriptorSetLayoutBinding(
				binding.binding,
				vulkan::to_vulkan_descriptor_type(binding.type),
				binding.descriptorCount,
				vulkan::to_vulkan_shader_stage(binding.shaderStages)
			));
		}

		vk::DescriptorSetLayoutCreateInfo createInfo{};
#if !defined(SPITE_USE_DESCRIPTOR_SETS)
		createInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT;
#endif
		createInfo.bindingCount = static_cast<u32>(bindings.size());
		createInfo.pBindings = bindings.data();

		auto newLayout = std::make_unique<VulkanDescriptorSetLayout>(renderContext.device, createInfo);

		u32 index;
		if (!m_layouts.freeIndices.empty())
		{
			index = m_layouts.freeIndices.back();
			m_layouts.freeIndices.pop_back();
			m_layouts.resources[index] = std::move(newLayout);
		}
		else
		{
			index = static_cast<u32>(m_layouts.resources.size());
			m_layouts.resources.push_back(std::move(newLayout));
			m_layouts.generations.push_back(0);
		}

		if (index >= m_layoutDescriptions.size())
		{
			m_layoutDescriptions.resize(index + 1);
		}
		m_layoutDescriptions[index] = description;

		ResourceSetLayoutHandle newHandle{index, m_layouts.generations[index]};
		m_cache[description] = newHandle;

		return newHandle;
	}

	IResourceSetLayout& VulkanResourceSetLayoutCache::getLayout(ResourceSetLayoutHandle handle)
	{
		SASSERT(
			handle.isValid() && handle.id < m_layouts.resources.size() && m_layouts.generations[handle.id] == handle.
			generation)
		return *m_layouts.resources[handle.id];
	}

	const IResourceSetLayout& VulkanResourceSetLayoutCache::getLayout(ResourceSetLayoutHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_layouts.resources.size() && m_layouts.generations[handle.id] == handle.
			generation)
		return *m_layouts.resources[handle.id];
	}

	const ResourceSetLayoutDescription& VulkanResourceSetLayoutCache::getLayoutDescription(ResourceSetLayoutHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_layoutDescriptions.size() && m_layouts.generations[handle.id] == handle.generation
		)
		return m_layoutDescriptions[handle.id];
	}
}
