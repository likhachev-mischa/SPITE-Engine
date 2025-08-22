#pragma once

#include "base/Platform.hpp"

namespace spite
{
	using ExecutionStage = u16;

	namespace CoreExecutionStages
	{
		enum : ExecutionStage
		{
			PRE_UPDATE = 10000,

			UPDATE = 20000,

			POST_UPDATE = 30000,

			PRE_RENDER = 40000,

			RENDER = 50000,

			POST_RENDER = 60000
		};
	}
}
