#pragma once

#ifdef SPITE_TEST
#include "ecs/config/TestComponents.hpp"
#else
#include "ecs/config/Components.hpp"
#endif


#include "ComponentMetadata.hpp"
#include "IComponent.hpp"
#include "base/Platform.hpp"
#include <EASTL/array.h>
#include <utility>
#include <type_traits>
#include <mutex>

#include "Base/Assert.hpp"


namespace spite
{
	class SharedComponentManager;

	// This context is passed to the policy function at runtime.
	class DestructionContext
	{
	private:
		SharedComponentManager* sharedManager;

	public:
		DestructionContext(SharedComponentManager* sharedManager): sharedManager(sharedManager)
		{
		}

		void destroySharedHandle(void* component) const;
	};

	namespace detail
	{
		// The default destruction policy: do nothing.
		inline void empty_destruction_policy(void* /*componentPtr*/, const DestructionContext& /*context*/)
		{
		}

		// Helper to find the compile-time index of a type T in a tuple.
		template <typename T, typename Tuple>
		struct TupleIndex;

		template <typename T, typename... Types>
		struct TupleIndex<T, std::tuple<T, Types...>>
		{
			static constexpr sizet value = 0;
		};

		template <typename T, typename U, typename... Types>
		struct TupleIndex<T, std::tuple<U, Types...>>
		{
			static constexpr sizet value = 1 + TupleIndex<T, std::tuple<Types...>>::value;
		};

		template <typename T, typename Tuple>
		constexpr sizet tuple_index_v = TupleIndex<T, Tuple>::value;

		template <typename Tuple, typename Func, std::size_t... Is>
		void for_each_in_tuple(Func&& func, std::index_sequence<Is...>)
		{
			(func.template operator()<std::tuple_element_t<Is, Tuple>>(), ...);
		}

		template <typename Tuple, typename Func>
		void for_each_in_tuple(Func&& func)
		{
			for_each_in_tuple<Tuple>(std::forward<Func>(func), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
		}

		// Creates a single ComponentMetadata entry for a given component type.
		template <t_component T>
		constexpr ComponentMetadata create_metadata_for()
		{
			const ComponentID newId = static_cast<ComponentID>(detail::tuple_index_v<T, ComponentList> + 1);

			// Start with the default no-op policy.
			ComponentMetadata::DestructionPolicyFn policyFn = &empty_destruction_policy;
			ComponentMetadata::MoveAndDestroyFn moveAndDestroyFn;

			// --- Overwrite Destruction Policy for Special Cases ---
			if constexpr (t_shared_handle<T>)
			{
				policyFn = [](void* c, const DestructionContext& ctx)
				{
					ctx.destroySharedHandle(c);
				};
			}
			else if constexpr (!std::is_trivially_destructible_v<T>)
			{
				policyFn = [](void* c, const DestructionContext&) { static_cast<T*>(c)->~T(); };
			}

			// --- Select Move Policy ---
			// This is the corrected condition for trivial relocatability in C++20
			if constexpr (std::is_trivially_move_constructible_v<T> && std::is_trivially_destructible_v<T>)
			{
				// Most optimal path: raw memory copy.
				moveAndDestroyFn = [](void* dest, void* src)
				{
					memcpy(dest, src, sizeof(T));
				};
			}
			else
			{
				// Safe path for non-trivial types.
				moveAndDestroyFn = [](void* dest, void* src)
				{
					new(dest) T(std::move(*static_cast<T*>(src)));
					static_cast<T*>(src)->~T();
				};
			}

			return ComponentMetadata(
				newId,
				sizeof(T),
				alignof(T),
				policyFn,
				moveAndDestroyFn
			);
		}
	}

	class ComponentMetadataRegistry
	{
	private:
		static constexpr ComponentID MAX_COMPONENTS = std::tuple_size_v<ComponentList> + 1;

		// The metadata table is now lazily initialized at runtime.
		static eastl::array<ComponentMetadata, MAX_COMPONENTS> m_idToMetadata;
		static std::once_flag m_onceFlag;

		// The initialization function, to be called only once.
		static void initialize();

	public:
		constexpr ComponentMetadataRegistry() = default;

		// Returns the unique, compile-time ID for a component type.
		template <t_component T>
		static constexpr ComponentID getComponentId()
		{
			return static_cast<ComponentID>(detail::tuple_index_v<T, ComponentList> + 1);
		}

		// Retrieves the metadata for a given component ID.
		static const ComponentMetadata& getMetadata(ComponentID id)
		{
			std::call_once(m_onceFlag, initialize);
			SASSERT(id < MAX_COMPONENTS)
			return m_idToMetadata[id];
		}

		// Retrieves the metadata for a given component type.
		template <t_component T>
		static const ComponentMetadata& getMetadata()
		{
			std::call_once(m_onceFlag, initialize);
			return m_idToMetadata[getComponentId<T>()];
		}
	};
}
