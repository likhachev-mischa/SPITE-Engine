#include "IResourceSetLayoutCache.hpp"

#include "base/CollectionUtilities.hpp"

namespace spite
{
	bool ResourceSetLayoutDescription::operator==(const ResourceSetLayoutDescription& other) const
	{
		return bindings == other.bindings;
	}

	sizet ResourceSetLayoutDescription::Hash::operator()(const ResourceSetLayoutDescription& desc) const
	{
		sizet seed = 0;
		for (const auto& binding : desc.bindings)
		{
			hashCombine(seed, binding.binding, binding.type, binding.descriptorCount, binding.shaderStages, binding.name, binding.size);
		}
		return seed;
	}
}
