#pragma once
#include <type_traits>

namespace spite
{
	struct IComponent
	{
		IComponent() = default;
		IComponent(const IComponent& other) = default;
		IComponent(IComponent&& other) noexcept = default;
		IComponent& operator=(const IComponent& other) = default;
		IComponent& operator=(IComponent&& other) noexcept = default;
		virtual ~IComponent() = default;
	};

	struct ISharedComponent : IComponent
	{
	};

	struct ISingletonComponent : IComponent
	{
	};

	struct IEventComponent : IComponent
	{
	};

	//components in general
	template <typename TComponent> concept t_component = std::is_base_of_v<IComponent, TComponent>
		&& std::is_default_constructible_v<TComponent> && std::is_move_assignable_v<TComponent> &&
		std::is_move_constructible_v<TComponent>;

	//specificly only IComponent
	template <typename TComponent> concept t_plain_component = std::is_base_of_v<
			IComponent, TComponent> && !std::is_base_of_v<ISharedComponent, TComponent> && !
		std::is_base_of_v<ISingletonComponent, TComponent> && !std::is_base_of_v<
			IEventComponent, TComponent> && std::is_default_constructible_v<TComponent> &&
		std::is_move_assignable_v<TComponent> && std::is_move_constructible_v<TComponent>;

	//specificly ISharedComponent
	template <typename TComponent> concept t_shared_component = std::is_base_of_v<
			ISharedComponent, TComponent> && !std::is_base_of_v<ISingletonComponent, TComponent> &&
		std::is_default_constructible_v<TComponent> && std::is_move_assignable_v<TComponent> &&
		std::is_move_constructible_v<TComponent>;

	//specificly ISingletonComponent
	template <typename TComponent> concept t_singleton_component = std::is_base_of_v<
			ISingletonComponent, TComponent> && !std::is_base_of_v<ISharedComponent, TComponent> &&
		std::is_default_constructible_v<TComponent> && std::is_move_assignable_v<TComponent> &&
		std::is_move_constructible_v<TComponent>;

	//specificly IEventComponent
	template <typename TComponent> concept t_event_component = std::is_base_of_v<
			IEventComponent, TComponent> && !std::is_base_of_v<ISharedComponent, TComponent> && !
		std::is_base_of_v<ISingletonComponent, TComponent> && std::is_default_constructible_v<
			TComponent> && std::is_move_assignable_v<TComponent> && std::is_move_constructible_v<
			TComponent>;

}
