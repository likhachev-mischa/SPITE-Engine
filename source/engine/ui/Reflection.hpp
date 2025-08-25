#pragma once

#include "base/CollectionAliases.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "ecs/core/IComponent.hpp"
#include "ecs/core/ComponentMetadataRegistry.hpp"

#include <string_view>
#include <typeindex>
#include <functional>

#include "base/CollectionUtilities.hpp"

namespace spite
{
	// Stores metadata for a single reflected member of a struct
	struct Member
	{
		std::string_view name;
		std::type_index type;
		sizet offset;
	};

	// Stores all reflected members for a single component type
	struct ComponentReflection
	{
		const char* name;
		heap_vector<Member> members;
	};

	template <t_component T>
	class ReflectionBuilder;

	// A registry to hold reflection data for all components
	class ReflectionRegistry
	{
	private:
		heap_unordered_map<ComponentID, ComponentReflection> m_reflections;
		HeapAllocator m_allocator;
		static ReflectionRegistry* m_instance;

	public:
		ReflectionRegistry(const HeapAllocator& allocator);

		static void init(HeapAllocator& allocator);
		static void destroy();
		static ReflectionRegistry* get();

		template <t_component T>
		ReflectionBuilder<T> reflect();

		const ComponentReflection* getReflection(ComponentID componentId) const;
	};

	// A builder class to provide the fluent registration syntax: reflect<T>().member(...);
	template <t_component T>
	class ReflectionBuilder
	{
	private:
		ComponentReflection* m_reflection;

	public:
		ReflectionBuilder(ComponentReflection* reflection) : m_reflection(reflection)
		{
		}

		template <typename MemberType>
		ReflectionBuilder<T>& member(std::string_view name, MemberType T::* ptr)
		{
			// A bit of a hack to get offset from a pointer-to-member
			T* base = nullptr;
			MemberType* member_ptr = &(base->*ptr);
			sizet offset = reinterpret_cast<sizet>(member_ptr);

			m_reflection->members.push_back({
				name,
				std::type_index(typeid(MemberType)),
				offset
			});
			return *this;
		}
	};

	template <t_component T>
	ReflectionBuilder<T> ReflectionRegistry::reflect()
	{
		const ComponentID id = ComponentMetadataRegistry::getComponentId<T>();
		auto it = m_reflections.find(id);
		if (it == m_reflections.end())
		{
			it = m_reflections.emplace(id, ComponentReflection{
				                           typeid(T).name(),
				                           makeHeapVector<Member>(m_allocator)
			                           }).first;
		}
		return ReflectionBuilder<T>(&it->second);
	}
}
