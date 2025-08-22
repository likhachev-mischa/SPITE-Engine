#pragma once

#include "ecs/systems/SystemBase.hpp"

namespace spite
{
	class TransformMatrixCalculateSystem : public SystemBase
	{
	public:
		QueryHandle query;

		void onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage) override;
		void onUpdate(SystemContext ctx) override;
	};
}
