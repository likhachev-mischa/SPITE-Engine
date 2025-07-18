#pragma once
#include <type_traits>
#include <utility>      

#include "base/Platform.hpp"

namespace spite
{
	// --- Base Component Interfaces ---

	struct IComponent
	{
	};

	// Is actually what user creates
	// A marker interface for data that can be stored in the SharedComponentManager.
	struct ISharedComponent
	{
	};

	struct ISingletonComponent
	{
	};

	// --- Component Concepts ---

	// A concept for any type that can be stored in a chunk.
	template <typename T>
	concept t_component = std::is_base_of_v<IComponent, T> && std::is_move_constructible_v<T>;

	// A concept for the user-defined shared data structs (e.g., Material, Texture).
	// Enforces that the type has nested Hash and Equals structs.
	template <typename T>
	concept t_shared_component = std::is_base_of_v<ISharedComponent, T> && std::is_copy_constructible_v<T> &&
		requires(T v) {
			{ typename T::Hash{}(v) } -> std::convertible_to<sizet>;
			{ typename T::Equals{}(v, v) } -> std::convertible_to<bool>;
		};

	template <typename T>
	concept t_singleton_component = std::is_base_of_v<ISingletonComponent, T> && std::is_default_constructible_v<T>;

	// Forward-declare the handle struct. The full definition is in SharedComponentManager.hpp
	struct SharedComponentHandle;

	// Code-generated boilerplate
	// The generic HANDLE component that will be stored in chunks for shared data.
	// This IS an IComponent.
	template <t_shared_component T>
	struct SharedComponent : IComponent
	{
		SharedComponentHandle handle;
	};

	// A helper to determine if a given type T is a specialization of SharedComponent<U>.
	template <typename T>
	struct is_shared_handle : std::false_type
	{
	};

	template <t_shared_component U>
	struct is_shared_handle<SharedComponent<U>> : std::true_type
	{
	};

	// A concept to specifically identify SharedComponent<T> handle components.
	template <typename T>
	concept t_shared_handle = is_shared_handle<T>::value;
}
