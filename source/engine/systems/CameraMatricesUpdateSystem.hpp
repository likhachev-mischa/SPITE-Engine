#pragma once

#include "ecs/systems/SystemBase.hpp"

namespace spite
{
	class CameraMatricesUpdateSystem : public SystemBase
	{
	public:
		void onUpdate(SystemContext ctx) override;
	};
}
