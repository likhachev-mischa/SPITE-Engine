#pragma once
#include "RenderResourceHandles.hpp"
#include "GraphicsTypes.hpp"
#include "base/CollectionAliases.hpp"
#include "base/HashedString.hpp"

namespace spite
{
    class IResourceSetLayout;

    struct ResourceBinding
    {
        u32 binding = 0;
        DescriptorType type;
        u32 descriptorCount = 1; // For arrays of resources
        ShaderStage shaderStages = ShaderStage::NONE;
        HashedString name; // For UBOs, this is the block name from the shader
        sizet size = 0;

        bool operator==(const ResourceBinding& other) const = default;
    };

    struct ResourceSetLayoutDescription
    {
        sbo_vector<ResourceBinding> bindings;

        bool operator==(const ResourceSetLayoutDescription& other) const;
        struct Hash { sizet operator()(const ResourceSetLayoutDescription& desc) const; };
    };

    class IResourceSetLayoutCache
    {
    public:
        virtual ~IResourceSetLayoutCache() = default;

        virtual ResourceSetLayoutHandle getOrCreateLayout(const ResourceSetLayoutDescription& description) = 0;

        virtual IResourceSetLayout& getLayout(ResourceSetLayoutHandle handle) = 0;
        virtual const IResourceSetLayout& getLayout(ResourceSetLayoutHandle handle) const = 0;

		virtual const ResourceSetLayoutDescription& getLayoutDescription(ResourceSetLayoutHandle handle) const = 0;
    };
}
