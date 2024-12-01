#pragma once
#include "Engine/Resources.hpp"


namespace spite
{
	class Renderer
	{
		Renderer(spite::HeapAllocator& allocator, const InstanceExtensions& extensions)
		{
			auto pResourceAllocation =
				std::make_shared<AllocationCallbacksWrapper>(allocator);

			auto pInstance = std::make_shared<InstanceWrapper>(allocator, pResourceAllocation,
			                                            extensions.extensions);
		}
	};
}
