#include "Common.hpp"

namespace spite
{
	bool QueueFamilyIndices::isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value() &&
			transferFamily.has_value();
	}
}
