#include "BeginFrameSystem.hpp"

#include "engine/components/RenderingComponents.hpp"
#include "engine/rendering/RenderingManager.hpp"

namespace spite
{
	void BeginFrameSystem::onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage)
	{
		setExecutionStage(CoreExecutionStages::POST_UPDATE);
	}

	void BeginFrameSystem::onUpdate(SystemContext ctx)
	{
		ctx.accessSingleton<RenderingManagerSingleton>([](RenderingManagerSingleton& singleton)
		{
			singleton.renderingManager->beginFrame();
		});
	}
}
