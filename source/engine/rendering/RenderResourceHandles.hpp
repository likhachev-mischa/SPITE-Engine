#pragma once
#include "base/Platform.hpp"
#include <compare>

namespace spite
{
	// A unique identifier for a resource within its pool.
	constexpr u32 INVALID_RESOURCE_ID = -1;

	// Generic handle structure, providing a common base for all resource handles.
	struct GpuResourceHandle
	{
		u32 id = INVALID_RESOURCE_ID;
		u32 generation = 0;

		[[nodiscard]] bool isValid() const { return id != INVALID_RESOURCE_ID; }

		auto operator<=>(const GpuResourceHandle& other) const = default;
	};

	// --- Type-Safe Handles ---
	// By creating distinct types, we prevent accidentally using one resource type's handle
	// where another is expected (e.g., using a texture handle to access a buffer).

	struct BufferHandle : GpuResourceHandle
	{
	};

	struct TextureHandle : GpuResourceHandle
	{
	};

	struct SamplerHandle : GpuResourceHandle
	{
	};

	struct PipelineHandle : GpuResourceHandle
	{
	};

	struct ImageViewHandle : GpuResourceHandle
	{
	};

	struct ResourceSetLayoutHandle : GpuResourceHandle
	{
	};

	struct RenderPassHandle : GpuResourceHandle
	{
	};

	struct FramebufferHandle : GpuResourceHandle
	{
	};

	struct ShaderModuleHandle : GpuResourceHandle
	{
	};

	struct PipelineLayoutHandle : GpuResourceHandle
	{
	};
}
