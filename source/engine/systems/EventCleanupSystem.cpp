#include "EventCleanupSystem.hpp"

namespace spite
{
	void EventCleanupSystem::onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage)
	{
		setExecutionStage(CoreExecutionStages::POST_RENDER);

		auto& queryDesc = ctx.getQueryBuilder().with<Read<EventTag>>();
		eventQuery = registerQuery(queryDesc, dependencyStorage);
		setPrerequisite(eventQuery);
	}

	void EventCleanupSystem::onUpdate(SystemContext ctx)
	{
		auto& cb = ctx.getCommandBuffer();
		for (auto [tag, entity] : eventQuery.view<Read<EventTag>, Entity>())
		{
			cb.destroyEntity(entity);
		}
	}
}
