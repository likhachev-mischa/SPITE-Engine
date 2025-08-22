#include "DepthPassSystem.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/RenderingComponents.hpp"
#include "engine/rendering/ISecondaryRenderCommandBuffer.hpp"
#include "engine/rendering/IRenderer.hpp"
#include "engine/rendering/RenderGraph.hpp"

namespace spite
{
	void DepthPassSystem::onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage)
	{
		setExecutionStage(CoreExecutionStages::PRE_RENDER);
		auto queryDescr = ctx.getQueryBuilder().with<Read<TransformMatrixComponent>, Read<MeshComponent>>();
		modelQuery = registerQuery(queryDescr, dependencyStorage);

		setPrerequisite(modelQuery);
	}

	void DepthPassSystem::onUpdate(SystemContext ctx)
	{
		const heap_string passName = "Depth";

		ISecondaryRenderCommandBuffer* cb;
		ctx.accessSingleton<RendererSingleton>([&cb,&passName](const RendererSingleton& singleton)
		{
			cb = singleton.renderer->acquireSecondaryCommandBuffer(passName);
		});

		PipelineLayoutHandle layout;
		ctx.accessSingleton<RenderGraphSingleton>([&layout,&passName](const RenderGraphSingleton& singleton)
		{
			layout = singleton.renderGraph->getPipelineLayoutForPass(passName);
		});

#if defined(SPITE_USE_DESCRIPTOR_SETS)
		ctx.accessSingleton<RenderGraphSingleton>([&](const RenderGraphSingleton& singleton)
		{
			auto& resourceSets = singleton.renderGraph->getPassResourceSets(passName);
			if (!resourceSets.empty())
			{
				cb->bindDescriptorSets(layout, 0, { resourceSets.begin(),resourceSets.end() });
			}
		});
#endif

		modelQuery.forEachConstChunk([&cb,&layout](const Chunk* chunk)
		{
			auto transforms = chunk->getComponents<TransformMatrixComponent>();
			auto meshes = chunk->getComponents<MeshComponent>();

			for (sizet i = 0; i < chunk->size(); ++i)
			{
				cb->pushConstants(layout, ShaderStage::VERTEX, 0, sizeof(TransformMatrixComponent::matrix),
				                  &transforms[i].matrix);

				cb->bindVertexBuffer(meshes[i].vertexBuffer);
				cb->bindIndexBuffer(meshes[i].indexBuffer);

				cb->drawIndexed(meshes[i].indexCount);
			}
		});
	}
}
