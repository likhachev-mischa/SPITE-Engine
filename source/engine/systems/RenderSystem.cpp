#include "RenderSystem.hpp"

#include "engine/components/RenderingComponents.hpp"
#include "engine/rendering/RenderingManager.hpp"

namespace spite
{
	void RenderSystem::onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage)
	{
		setExecutionStage(CoreExecutionStages::RENDER);
	}

	void RenderSystem::onUpdate(SystemContext ctx)
	{
		ctx.accessSingleton<RenderingManagerSingleton>([](RenderingManagerSingleton& singleton)
		{
			singleton.renderingManager->render();
		});
	}
}
