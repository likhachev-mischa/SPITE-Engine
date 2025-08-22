#pragma once
#include "RenderResourceHandles.hpp"
#include "base/CollectionAliases.hpp"
#include "GraphicsDescs.hpp"

namespace spite
{
    class IPipelineLayout;

    struct PipelineLayoutDescription
    {
        sbo_vector<ResourceSetLayoutHandle> resourceSetLayouts;
        sbo_vector<PushConstantRange> pushConstantRanges;

        bool operator==(const PipelineLayoutDescription& other) const;

        struct Hash
        {
            sizet operator()(const PipelineLayoutDescription& desc) const;
        };
    };

    class IPipelineLayoutCache
    {
    public:
        virtual ~IPipelineLayoutCache() = default;

        virtual PipelineLayoutHandle getOrCreatePipelineLayout(const PipelineLayoutDescription& description) = 0;
        virtual void destroyPipelineLayout(PipelineLayoutHandle handle) = 0;

        virtual IPipelineLayout& getPipelineLayout(PipelineLayoutHandle handle) = 0;
        virtual const IPipelineLayout& getPipelineLayout(PipelineLayoutHandle handle) const = 0;
    };
}
