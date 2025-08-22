#pragma once
#include "ecs/systems/SystemBase.hpp"

namespace spite
{
	class EventCleanupSystem : public SystemBase
	{
	public:
		QueryHandle eventQuery;
		void onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage) override;
		void onUpdate(SystemContext ctx) override;
	};
}
