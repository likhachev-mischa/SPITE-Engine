#pragma once
#include "RenderResourceHandles.hpp"
#include "base/CollectionAliases.hpp"
#include "GraphicsTypes.hpp"

namespace spite
{
    class IShaderModule;

    struct ShaderDescription
    {
        heap_string path;
        ShaderStage stage;

        bool operator==(const ShaderDescription& other) const;

        struct Hash
        {
            sizet operator()(const ShaderDescription& desc) const;
        };
    };

    class IShaderModuleCache
    {
    public:
        virtual ~IShaderModuleCache() = default;

        virtual ShaderModuleHandle getOrCreateShaderModule(const ShaderDescription& description) = 0;
        virtual void destroyShaderModule(ShaderModuleHandle handle) = 0;

        virtual IShaderModule& getShaderModule(ShaderModuleHandle handle) = 0;
        virtual const IShaderModule& getShaderModule(ShaderModuleHandle handle) const = 0;
    };
}
