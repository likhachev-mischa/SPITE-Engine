#pragma once
#include <memory>

#include "base/CollectionAliases.hpp"
#include "engine/rendering/IShaderModuleCache.hpp"
#include "engine/rendering/ResourcePool.hpp"
#include "VulkanShaderReflectionModule.hpp" 

namespace spite
{
	struct VulkanRenderContext;
    class VulkanShaderModule;

    struct CachedShader
    {
        std::unique_ptr<VulkanShaderModule> module;
        ShaderReflectionData reflectionData;
    };

	class VulkanShaderModuleCache : public IShaderModuleCache
	{
		VulkanRenderContext& m_context;
		HeapAllocator m_allocator;
		heap_unordered_map<ShaderDescription, ShaderModuleHandle, ShaderDescription::Hash> m_cache;
		ResourcePool<CachedShader> m_shaderModules;
		heap_vector<ShaderDescription> m_shaderDescriptions;

	public:
		VulkanShaderModuleCache(VulkanRenderContext& context, const HeapAllocator& allocator);
		~VulkanShaderModuleCache() override;

		VulkanShaderModuleCache(const VulkanShaderModuleCache&) = delete;
		VulkanShaderModuleCache& operator=(const VulkanShaderModuleCache&) = delete;
		VulkanShaderModuleCache(VulkanShaderModuleCache&&) = delete;
		VulkanShaderModuleCache& operator=(VulkanShaderModuleCache&&) = delete;

		ShaderModuleHandle getOrCreateShaderModule(const ShaderDescription& description) override;
		void destroyShaderModule(ShaderModuleHandle handle) override;

		IShaderModule& getShaderModule(ShaderModuleHandle handle) override;
		const IShaderModule& getShaderModule(ShaderModuleHandle handle) const override;
        const ShaderReflectionData& getReflectionData(ShaderModuleHandle handle) const;
	};
}
