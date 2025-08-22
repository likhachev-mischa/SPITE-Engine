#include "VulkanPipelineLayoutCache.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanResourceSetLayoutCache.hpp"
#include "VulkanPipelineLayout.hpp"
#include "VulkanDescriptorSetLayout.hpp"
#include "base/Assert.hpp"
#include "base/CollectionUtilities.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanTypeMappings.hpp"
#include <memory>

namespace spite
{
	bool PipelineLayoutDescription::operator==(const PipelineLayoutDescription& other) const
	{
		return resourceSetLayouts == other.resourceSetLayouts && pushConstantRanges == other.pushConstantRanges;
	}

	sizet PipelineLayoutDescription::Hash::operator()(const PipelineLayoutDescription& desc) const
	{
		sizet seed = 0;
		for (const auto& handle : desc.resourceSetLayouts)
		{
			hashCombine(seed, handle.id, handle.generation);
		}
		for (const auto& range : desc.pushConstantRanges)
		{
			hashCombine(seed, range.shaderStages, range.offset, range.size);
		}
		return seed;
	}

	VulkanPipelineLayoutCache::VulkanPipelineLayoutCache(VulkanRenderDevice& device, const HeapAllocator& allocator)
		: m_device(device),
		  m_allocator(allocator),
		  m_cache(
			  makeHeapMap<PipelineLayoutDescription, PipelineLayoutHandle, PipelineLayoutDescription::Hash>(allocator)),
		  m_pipelineLayouts(allocator),
		  m_pipelineLayoutDescriptions(makeHeapVector<PipelineLayoutDescription>(allocator))
	{
	}

	VulkanPipelineLayoutCache::~VulkanPipelineLayoutCache()
	{
	}

	PipelineLayoutHandle VulkanPipelineLayoutCache::getOrCreatePipelineLayout(
		const PipelineLayoutDescription& description)
	{
		auto it = m_cache.find(description);
		if (it != m_cache.end())
		{
			return it->second;
		}

		auto& renderContext = m_device.getContext();
		auto& resourceSetLayoutCache = m_device.getResourceSetLayoutCache();

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto setLayouts = makeScratchVector<vk::DescriptorSetLayout>(FrameScratchAllocator::get());
		for (const auto& handle : description.resourceSetLayouts)
		{
			VulkanDescriptorSetLayout& layout = static_cast<VulkanDescriptorSetLayout&>(resourceSetLayoutCache.
				getLayout(handle));
			setLayouts.push_back(layout.get());
		}

		auto pushConstantRanges = makeScratchVector<vk::PushConstantRange>(FrameScratchAllocator::get());
		for (const auto& range : description.pushConstantRanges)
		{
			pushConstantRanges.push_back(vk::PushConstantRange(
				vulkan::to_vulkan_shader_stage(range.shaderStages),
				range.offset,
				range.size
			));
		}

		vk::PipelineLayoutCreateInfo createInfo{};
		createInfo.setLayoutCount = static_cast<u32>(setLayouts.size());
		createInfo.pSetLayouts = setLayouts.data();
		createInfo.pushConstantRangeCount = static_cast<u32>(pushConstantRanges.size());
		createInfo.pPushConstantRanges = pushConstantRanges.data();

		auto newLayout = std::make_unique<VulkanPipelineLayout>(renderContext.device, createInfo);

		u32 index;
		if (!m_pipelineLayouts.freeIndices.empty())
		{
			index = m_pipelineLayouts.freeIndices.back();
			m_pipelineLayouts.freeIndices.pop_back();
			m_pipelineLayouts.resources[index] = std::move(newLayout);
			m_pipelineLayoutDescriptions[index] = description;
		}
		else
		{
			index = static_cast<u32>(m_pipelineLayouts.resources.size());
			m_pipelineLayouts.resources.push_back(std::move(newLayout));
			m_pipelineLayouts.generations.push_back(0);
			m_pipelineLayoutDescriptions.push_back(description);
		}

		PipelineLayoutHandle newHandle{index, m_pipelineLayouts.generations[index]};
		m_cache[description] = newHandle;

		return newHandle;
	}

	void VulkanPipelineLayoutCache::destroyPipelineLayout(PipelineLayoutHandle handle)
	{
		if (!handle.isValid() || handle.id >= m_pipelineLayouts.resources.size() || m_pipelineLayouts.generations[handle
			.id] != handle.generation)
		{
			SASSERTM(false, "Attempted to destroy a pipeline layout with an invalid or stale handle.")
			return;
		}

		const auto& description = m_pipelineLayoutDescriptions[handle.id];
		m_cache.erase(description);

		m_pipelineLayouts.resources[handle.id].reset();
		m_pipelineLayouts.freeIndices.push_back(handle.id);
		m_pipelineLayouts.generations[handle.id]++;
	}

	IPipelineLayout& VulkanPipelineLayoutCache::getPipelineLayout(PipelineLayoutHandle handle)
	{
		SASSERT(
			handle.isValid() && handle.id < m_pipelineLayouts.resources.size() && m_pipelineLayouts.generations[handle.
				id] == handle.generation)
		return *m_pipelineLayouts.resources[handle.id];
	}

	const IPipelineLayout& VulkanPipelineLayoutCache::getPipelineLayout(PipelineLayoutHandle handle) const
	{
		SASSERT(
			handle.isValid() && handle.id < m_pipelineLayouts.resources.size() && m_pipelineLayouts.generations[handle.
				id] == handle.generation)
		return *m_pipelineLayouts.resources[handle.id];
	}
}
