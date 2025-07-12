#pragma once
#include <typeindex>

#include "Base/Platform.hpp"

namespace spite
{
	using ComponentID = u32;
	constexpr ComponentID INVALID_COMPONENT_ID = 0;

	struct ComponentMetadata
	{
		// The user data pointer allows passing context (like the SharedComponentManager) to the destructor.
		using DestructorFn = void (*)(void* componentPtr, void* userData);
		using MoveAndDestroyFn = void (*)(void* destPtr, void* srcPtr);

		ComponentID id = INVALID_COMPONENT_ID;
		std::type_index type = typeid(nullptr_t);
		sizet size = 0;
		sizet alignment = 0;
		bool isTriviallyRelocatable = true;
		
		DestructorFn destructor = nullptr;
		void* destructorUserData = nullptr; // User data for the destructor

		MoveAndDestroyFn moveAndDestroy = nullptr;

		ComponentMetadata() = default;

		ComponentMetadata(ComponentID id,
						  std::type_index type,
		                  sizet size,
		                  sizet alignment,
		                  bool isTriviallyRelocatable,
		                  DestructorFn destructorFn,
						  void* destructorUserData,
		                  MoveAndDestroyFn moveAndDestroyFn);
	};
}