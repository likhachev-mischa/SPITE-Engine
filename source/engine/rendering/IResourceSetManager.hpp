#pragma once
#include "RenderResourceHandles.hpp"
#include "GraphicsTypes.hpp"
#include "base/CollectionAliases.hpp"

namespace spite
{
    class IResourceSet;

    struct ResourceSetHandle : GpuResourceHandle {};

    struct ResourceWrite
    {
        u32 binding;
        u32 arrayElement = 0;
        DescriptorType type;
        GpuResourceHandle resource; // BufferHandle, ImageViewHandle, or SamplerHandle
    };

    // Manages the allocation and updating of ResourceSets.
    // This can be backed by either descriptor buffers or traditional descriptor sets,
    // controlled by the SPITE_USE_DESCRIPTOR_SETS preprocessor definition.
    class IResourceSetManager
    {
    public:
        virtual ~IResourceSetManager() = default;

        // Allocates a resource set for a given layout.
        virtual ResourceSetHandle allocateSet(ResourceSetLayoutHandle layoutHandle) = 0;

        // Writes the actual descriptor data for the given resources into the resource set.
        virtual void updateSet(ResourceSetHandle setHandle, const sbo_vector<ResourceWrite>& writes) = 0;

#if !defined(SPITE_USE_DESCRIPTOR_SETS)
        // Retrieves the underlying buffer used for a specific type of descriptor.
        virtual BufferHandle getDescriptorBuffer(DescriptorType type) const = 0;

        // Retrieves the size of a single descriptor for a given type.
        virtual sizet getDescriptorSize(DescriptorType type) const = 0;
#endif

        virtual IResourceSet& getSet(ResourceSetHandle handle) = 0;
        virtual const IResourceSet& getSet(ResourceSetHandle handle) const = 0;
    };
}