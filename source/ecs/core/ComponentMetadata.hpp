#pragma once
#include "Base/Platform.hpp"

namespace spite
{
	// Forward-declare for the function pointer signature
	class DestructionContext; 

	using ComponentID = u32;
	constexpr ComponentID INVALID_COMPONENT_ID = 0;

	struct ComponentMetadata
	{
		// A single function pointer to handle all destruction side-effects.
		using DestructionPolicyFn = void (*)(void* componentPtr, const DestructionContext& context);
		using MoveAndDestroyFn = void (*)(void* destPtr, void* srcPtr);

		ComponentID id = INVALID_COMPONENT_ID;
		sizet size = 0;
		sizet alignment = 0;
		
		DestructionPolicyFn destructionPolicy = nullptr;
		MoveAndDestroyFn moveAndDestroy = nullptr;

		constexpr ComponentMetadata() = default;

		constexpr ComponentMetadata(ComponentID id,
		                  sizet size,
		                  sizet alignment,
		                  DestructionPolicyFn policyFn,
		                  MoveAndDestroyFn moveAndDestroyFn)
			: id(id), size(size), alignment(alignment),
			  destructionPolicy(policyFn),
			  moveAndDestroy(moveAndDestroyFn)
		{}
	};
}
