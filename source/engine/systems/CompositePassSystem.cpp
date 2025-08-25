#include "CompositePassSystem.hpp"

#include "engine/components/RenderingComponents.hpp"
#include "engine/rendering/IRenderer.hpp"
#include "engine/rendering/ISecondaryRenderCommandBuffer.hpp"
#include "engine/rendering/RenderGraph.hpp"

namespace spite
{
	void CompositePassSystem::onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage)
	{
		setExecutionStage(CoreExecutionStages::PRE_RENDER);
	}

	void CompositePassSystem::onUpdate(SystemContext ctx)
	{
		const heap_string passName = "Composite";

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
				cb->bindDescriptorSets(layout, 0, {resourceSets.begin(), resourceSets.end()});
			}
		});
#endif

		cb->draw(3);
	}
}
