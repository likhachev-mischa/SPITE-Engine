#include "IShaderModuleCache.hpp"
#include "base/CollectionUtilities.hpp"

namespace spite
{
    bool ShaderDescription::operator==(const ShaderDescription& other) const
    {
        return path == other.path && stage == other.stage;
    }

    sizet ShaderDescription::Hash::operator()(const ShaderDescription& desc) const
    {
        sizet seed = 0;
        hashCombine(seed, eastl::hash<heap_string>()(desc.path), desc.stage);
        return seed;
    }
}
