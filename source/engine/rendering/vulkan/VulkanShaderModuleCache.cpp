#include "VulkanShaderModuleCache.hpp"

#include <memory>

#include "VulkanRenderContext.hpp"
#include "VulkanShaderModule.hpp"
#include "VulkanShaderReflectionModule.hpp"

#include "base/Assert.hpp"
#include "base/File.hpp"

namespace spite
{

    VulkanShaderModuleCache::VulkanShaderModuleCache(VulkanRenderContext& context, const HeapAllocator& allocator)
        : m_context(context),
          m_allocator(allocator),
          m_cache(makeHeapMap<ShaderDescription, ShaderModuleHandle, ShaderDescription::Hash>(allocator)),
          m_shaderModules(allocator),
          m_shaderDescriptions(makeHeapVector<ShaderDescription>(allocator))
    {}

    VulkanShaderModuleCache::~VulkanShaderModuleCache()
    {
        //for (auto& cachedShader : m_shaderModules.resources)
        //{
            //cachedShader.~CachedShader();
        //}
    }

    ShaderModuleHandle VulkanShaderModuleCache::getOrCreateShaderModule(const ShaderDescription& description)
    {
        auto it = m_cache.find(description);
        if (it != m_cache.end())
        {
            return it->second;
        }

        auto& renderContext = m_context;
        auto shaderCode = readBinaryFile(description.path.c_str());
        SASSERTM(!shaderCode.empty(), "Failed to read shader file")

        auto newModule = std::make_unique<VulkanShaderModule>(renderContext.device, shaderCode);

        VulkanShaderReflectionModule reflector(shaderCode, description.stage);
        ShaderReflectionData reflectionData = reflector.reflect();

        CachedShader newCachedShader;
        newCachedShader.module = std::move(newModule);
        newCachedShader.reflectionData = std::move(reflectionData);

        u32 index;
        if (!m_shaderModules.freeIndices.empty()) {
            index = m_shaderModules.freeIndices.back();
            m_shaderModules.freeIndices.pop_back();
            m_shaderModules.resources[index] = std::move(newCachedShader);
            m_shaderDescriptions[index] = description;
        } else {
            index = static_cast<u32>(m_shaderModules.resources.size());
            m_shaderModules.resources.push_back(std::move(newCachedShader));
            m_shaderModules.generations.push_back(0);
            m_shaderDescriptions.push_back(description);
        }
        
        ShaderModuleHandle newHandle{ index, m_shaderModules.generations[index] };
        m_cache[description] = newHandle;

        return newHandle;
    }

    void VulkanShaderModuleCache::destroyShaderModule(ShaderModuleHandle handle)
    {
        if (!handle.isValid() || handle.id >= m_shaderModules.resources.size() || m_shaderModules.generations[handle.id] != handle.generation)
        {
            SASSERTM(false, "Attempted to destroy a shader module with an invalid or stale handle.")
            return;
        }

        const auto& description = m_shaderDescriptions[handle.id];
        m_cache.erase(description);

        m_shaderModules.resources[handle.id].~CachedShader();
        m_shaderModules.freeIndices.push_back(handle.id);
        m_shaderModules.generations[handle.id]++;
    }

    IShaderModule& VulkanShaderModuleCache::getShaderModule(ShaderModuleHandle handle)
    {
        SASSERT(handle.isValid() && handle.id < m_shaderModules.resources.size() && m_shaderModules.generations[handle.id] == handle.generation)
        return *m_shaderModules.resources[handle.id].module;
    }

    const IShaderModule& VulkanShaderModuleCache::getShaderModule(ShaderModuleHandle handle) const
    {
        SASSERT(handle.isValid() && handle.id < m_shaderModules.resources.size() && m_shaderModules.generations[handle.id] == handle.generation)
        return *m_shaderModules.resources[handle.id].module;
    }

    const ShaderReflectionData& VulkanShaderModuleCache::getReflectionData(ShaderModuleHandle handle) const
    {
        SASSERT(handle.isValid() && handle.id < m_shaderModules.resources.size() && m_shaderModules.generations[handle.id] == handle.generation)
        return m_shaderModules.resources[handle.id].reflectionData;
    }
}
