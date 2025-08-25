#pragma once
#include "ecs/core/Entity.hpp"
#include "ecs/core/IComponent.hpp"

namespace spite
{
	struct InspectedEntitySingleton : ISingletonComponent
	{
		Entity entity;
	};
}
