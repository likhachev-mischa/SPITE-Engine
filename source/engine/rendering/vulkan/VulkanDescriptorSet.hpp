#pragma once
#include "base/VulkanUsage.hpp"
#include "engine/rendering/IResourceSet.hpp"

namespace spite
{
    struct VulkanDescriptorSet : public IResourceSet
    {
        vk::DescriptorSet set;
        vk::DescriptorSet getSet() const { return set; }
    };
}