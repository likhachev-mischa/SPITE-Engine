#pragma once

#ifdef SPITE_TEST
#include "ecs/config/TestComponents.hpp"
#else
#include "ecs/config/SingletonComponents.hpp"
#endif

#include <memory>
#include <tuple>
#include <utility>

#include "base/Assert.hpp"

#include "ecs/core/ComponentMetadataRegistry.hpp"
#include "ecs/core/IComponent.hpp"

namespace spite
{
	class SingletonComponentRegistry
	{
	private:
		// Helper to create a tuple of unique_ptrs from a tuple of types.
		template <typename... Ts>
		static auto create_instance_tuple(const std::tuple<Ts...>&)
		{
			return std::make_tuple(std::unique_ptr<Ts>()...);
		}

		// The storage for all singleton instances.
		decltype(create_instance_tuple(SingletonComponentList{})) m_instances;

	public:
		SingletonComponentRegistry() = default;

		// This registry manages unique resources and should not be copied or moved.
		SingletonComponentRegistry(const SingletonComponentRegistry&) = delete;
		SingletonComponentRegistry& operator=(const SingletonComponentRegistry&) = delete;
		SingletonComponentRegistry(SingletonComponentRegistry&&) = delete;
		SingletonComponentRegistry& operator=(SingletonComponentRegistry&&) = delete;

		~SingletonComponentRegistry() = default;

		// Gets the singleton instance.
		// Creates it on first access if it doesn't exist.
		template <t_singleton_component T>
		T& get()
		{
			constexpr auto componentId = detail::tuple_index_v<T, SingletonComponentList>;
			auto& ptr = std::get<componentId>(m_instances);

			if (!ptr)
			{
				// Lazy instantiation on first call
				ptr = std::make_unique<T>();
			}
			return *ptr;
		}

		template <t_singleton_component T>
		const T& get() const
		{
			constexpr auto componentId = detail::tuple_index_v<T, SingletonComponentList>;
			const auto& ptr = std::get<componentId>(m_instances);
			SASSERTM(ptr, "Singleton of type %s was accessed before it was created.", typeid(T).name())
			return *ptr;
		}

		// Allows for custom initialization or mocking for tests.
		// This will overwrite any existing instance.
		template <t_singleton_component T>
		void registerSingleton(std::unique_ptr<T> instance)
		{
			constexpr auto componentId = detail::tuple_index_v<T, SingletonComponentList>;
			auto& ptr = std::get<componentId>(m_instances);
			ptr = std::move(instance);
		}
	};
}
