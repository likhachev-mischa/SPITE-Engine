#pragma once
#include "ecs/Core.hpp"

namespace spite
{
	struct MovementButtonPressEvent : IEventComponent
	{
		float value;
	};
}
