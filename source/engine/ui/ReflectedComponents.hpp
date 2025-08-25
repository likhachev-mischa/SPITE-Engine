#pragma once

#include "engine/ui/Reflection.hpp"
#include "engine/components/CoreComponents.hpp"

namespace spite
{
	// A single function to register all components we want to inspect.
	// This should be called once at startup after the registries are initialized.
	inline void registerReflectedComponents()
	{
		auto* registry = ReflectionRegistry::get();

		registry->reflect<TransformComponent>()
		        .member("position", &TransformComponent::position)
		        .member("rotation", &TransformComponent::rotation)
		        .member("scale", &TransformComponent::scale);

		// When you create new components, you can add their reflection here.
		// For example:
		// registry->reflect<MyNewComponent>()
		// 	.member("some_float", &MyNewComponent::some_float)
		// 	.member("some_bool", &MyNewComponent::some_bool);
	}
}
