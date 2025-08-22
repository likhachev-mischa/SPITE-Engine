#pragma once
#include "engine/rendering/IPipelineLayoutCache.hpp"
#include "engine/rendering/ResourcePool.hpp"
#include "base/CollectionAliases.hpp"
#include <memory>

namespace spite
{
    class VulkanRenderDevice;

    class VulkanPipelineLayoutCache : public IPipelineLayoutCache
    {
        VulkanRenderDevice& m_device;
        HeapAllocator m_allocator;
        heap_unordered_map<PipelineLayoutDescription, PipelineLayoutHandle, PipelineLayoutDescription::Hash> m_cache;
        ResourcePool<std::unique_ptr<IPipelineLayout>> m_pipelineLayouts;
        heap_vector<PipelineLayoutDescription> m_pipelineLayoutDescriptions;

    public:
        VulkanPipelineLayoutCache(VulkanRenderDevice& device, const HeapAllocator& allocator);
        ~VulkanPipelineLayoutCache() override;

        VulkanPipelineLayoutCache(const VulkanPipelineLayoutCache&) = delete;
        VulkanPipelineLayoutCache& operator=(const VulkanPipelineLayoutCache&) = delete;
        VulkanPipelineLayoutCache(VulkanPipelineLayoutCache&&) = delete;
        VulkanPipelineLayoutCache& operator=(VulkanPipelineLayoutCache&&) = delete;

        PipelineLayoutHandle getOrCreatePipelineLayout(const PipelineLayoutDescription& description) override;
        void destroyPipelineLayout(PipelineLayoutHandle handle) override;

        IPipelineLayout& getPipelineLayout(PipelineLayoutHandle handle) override;
        const IPipelineLayout& getPipelineLayout(PipelineLayoutHandle handle) const override;
    };
}
