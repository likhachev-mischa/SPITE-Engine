#pragma once

#include "ecs/systems/SystemBase.hpp"

namespace spite
{
	class CompositePassSystem : public SystemBase
	{
	public:
		void onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage) override;
		void onUpdate(SystemContext ctx) override;
		
	};
}
