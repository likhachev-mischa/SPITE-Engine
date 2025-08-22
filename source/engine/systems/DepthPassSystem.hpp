#pragma once
#include "ecs/systems/SystemBase.hpp"

namespace spite
{
	class DepthPassSystem : public SystemBase
	{
	public:
		QueryHandle modelQuery;

		void onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage) override;
		void onUpdate(SystemContext ctx) override;
	};
}
