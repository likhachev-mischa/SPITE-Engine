#pragma once
#include "RenderResourceHandles.hpp"
#include "GraphicsTypes.hpp"

#include "base/HashedString.hpp"
#include "base/CollectionUtilities.hpp"

namespace spite
{
	class IShaderModule;

	struct ShaderDescription
	{
		HashedString path;
		ShaderStage stage;

		bool operator==(const ShaderDescription& other) const
		{
			return path == other.path && stage == other.stage;
		}

		struct Hash
		{
			sizet operator()(const ShaderDescription& desc) const
			{
				sizet seed = 0;
				hashCombine(seed, desc.path, desc.stage);
				return seed;
			}
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
