#pragma once

#include "base/memory/HeapAllocator.hpp"
#include "base/CollectionAliases.hpp"
#include <functional>
#include <typeindex>

namespace spite
{
	// A registry for functions that can draw ImGui UIs for specific data types.
	class TypeInspectorRegistry
	{
	private:
		// The function takes a label and a type-erased pointer to the data.
		// It should return true if the data was modified by the UI.
		using InspectorFn = std::function<bool(const char* name, void* data)>;

		static TypeInspectorRegistry* m_instance;
		heap_unordered_map<std::type_index, InspectorFn,std::hash<std::type_index>> m_inspectors;
		HeapAllocator m_allocator;

	public:
		TypeInspectorRegistry(const HeapAllocator& allocator);

		static void init(HeapAllocator& allocator);
		static void destroy();
		static TypeInspectorRegistry* get();

		template <typename T>
		void registerInspector(InspectorFn inspector)
		{
			m_inspectors[std::type_index(typeid(T))] = inspector;
		}

		// Draws the inspector UI for a given type.
		// Returns true if an inspector was found and drawn.
		bool inspect(const std::type_index& type, const char* name, void* data);
	};
}
