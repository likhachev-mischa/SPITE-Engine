#pragma once
#include "ecs/systems/SystemBase.hpp"

namespace spite
{
	class ModelLoadSystem : public SystemBase
	{
	public:
		QueryHandle requestQuery;

		void onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage) override;
		void onUpdate(SystemContext ctx) override;
	};
}
