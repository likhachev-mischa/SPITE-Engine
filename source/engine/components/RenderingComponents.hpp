#pragma once
#include "base/HashedString.hpp"

#include "ecs/core/IComponent.hpp"

#include "engine/rendering/RenderResourceHandles.hpp"

namespace spite
{
	class RenderGraph;
	class RenderingManager;
	class IRenderDevice;
	class IRenderer;
	class IRenderResourceManager;

	struct MeshComponent : IComponent
	{
		BufferHandle indexBuffer;
		BufferHandle vertexBuffer;
		u32 indexCount;
	};

	struct RenderGraphSingleton : ISingletonComponent
	{
		RenderGraph* renderGraph;
	};

	struct RenderingManagerSingleton : ISingletonComponent
	{
		RenderingManager* renderingManager;
	};

	struct RendererSingleton : ISingletonComponent
	{
		IRenderer* renderer;
	};

	struct RenderDeviceSingleton : ISingletonComponent
	{
		IRenderDevice* renderDevice;
	};

	struct RenderResourceManagerSingleton : ISingletonComponent
	{
		IRenderResourceManager* resourceManager;
	};

	struct ModelLoadRequest : IEventComponent
	{
		HashedString filePath;
	};
}
