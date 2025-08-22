#pragma once
#include "engine/rendering/IResourceSetLayoutCache.hpp"
#include "engine/rendering/ResourcePool.hpp"
#include "base/CollectionAliases.hpp"
#include <memory>

namespace spite
{
    struct VulkanRenderContext;

    class VulkanResourceSetLayoutCache : public IResourceSetLayoutCache
    {
    private:
        VulkanRenderContext& m_context;
        HeapAllocator m_allocator;
        heap_unordered_map<ResourceSetLayoutDescription, ResourceSetLayoutHandle, ResourceSetLayoutDescription::Hash> m_cache;
        ResourcePool<std::unique_ptr<IResourceSetLayout>> m_layouts;
		heap_vector<ResourceSetLayoutDescription> m_layoutDescriptions;

    public:
        VulkanResourceSetLayoutCache(VulkanRenderContext& context, const HeapAllocator& allocator);
        ~VulkanResourceSetLayoutCache() override;

        VulkanResourceSetLayoutCache(const VulkanResourceSetLayoutCache& other) = delete;
        VulkanResourceSetLayoutCache(VulkanResourceSetLayoutCache&& other) noexcept = delete;
        VulkanResourceSetLayoutCache& operator=(const VulkanResourceSetLayoutCache& other) = delete;
        VulkanResourceSetLayoutCache& operator=(VulkanResourceSetLayoutCache&& other) noexcept = delete;

        ResourceSetLayoutHandle getOrCreateLayout(const ResourceSetLayoutDescription& description) override;

        IResourceSetLayout& getLayout(ResourceSetLayoutHandle handle) override;
        const IResourceSetLayout& getLayout(ResourceSetLayoutHandle handle) const override;

		const ResourceSetLayoutDescription& getLayoutDescription(ResourceSetLayoutHandle handle) const override;
    };
}
