#pragma once
#include "engine/rendering/IPipelineCache.hpp"
#include "engine/rendering/ResourcePool.hpp"
#include "base/CollectionAliases.hpp"
#include <memory>

namespace spite
{
    class VulkanRenderDevice;

    class VulkanPipelineCache : public IPipelineCache
    {
        VulkanRenderDevice& m_device;
		HeapAllocator m_allocator;
		heap_unordered_map<PipelineDescription, PipelineHandle, PipelineDescription::Hash> m_cache;
		ResourcePool<std::unique_ptr<IPipeline>> m_pipelines;
		heap_vector<PipelineDescription> m_pipelineDescriptions;

    public:
        VulkanPipelineCache(VulkanRenderDevice& device, const HeapAllocator& allocator);
        ~VulkanPipelineCache() override;

        VulkanPipelineCache(const VulkanPipelineCache&) = delete;
        VulkanPipelineCache& operator=(const VulkanPipelineCache&) = delete;

        VulkanPipelineCache(VulkanPipelineCache&& other) = delete;

        VulkanPipelineCache& operator=(VulkanPipelineCache&& other) = delete;

        PipelineHandle getOrCreatePipeline(const PipelineDescription& description) override;

        PipelineHandle getOrCreatePipeline(const ComputePipelineDescription& description) override;

        PipelineHandle getOrCreatePipelineFromShaders(
			eastl::span<const ShaderStageDescription> shaderStages,
			const PipelineDescription& baseDescription
		) override;

        void destroyPipeline(PipelineHandle handle) override;

        IPipeline& getPipeline(PipelineHandle handle) override;

        const IPipeline& getPipeline(PipelineHandle handle) const override;

        const PipelineDescription& getPipelineDescription(PipelineHandle handle) const override;
		PipelineLayoutHandle getPipelineLayoutHandle(PipelineHandle handle) const override;

		void clear();
    };
}
