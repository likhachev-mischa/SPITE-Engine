#include "Reflection.hpp"
#include "base/Assert.hpp"

namespace spite
{
	ReflectionRegistry* ReflectionRegistry::m_instance = nullptr;

	void ReflectionRegistry::init(HeapAllocator& allocator)
	{
		SASSERTM(!m_instance, "ReflectionRegistry is already initialized");
		m_instance = allocator.new_object<ReflectionRegistry>(allocator);
	}

	void ReflectionRegistry::destroy()
	{
		SASSERTM(m_instance, "ReflectionRegistry is not initialized");
		m_instance->m_allocator.delete_object(m_instance);
		m_instance = nullptr;
	}

	ReflectionRegistry* ReflectionRegistry::get()
	{
		SASSERTM(m_instance, "ReflectionRegistry is not initialized");
		return m_instance;
	}

	ReflectionRegistry::ReflectionRegistry(const HeapAllocator& allocator)
		: m_reflections(makeHeapMap<ComponentID, ComponentReflection>(allocator)),
		  m_allocator(allocator)
	{
	}

	const ComponentReflection* ReflectionRegistry::getReflection(ComponentID componentId) const
	{
		auto it = m_reflections.find(componentId);
		if (it != m_reflections.end())
		{
			return &it->second;
		}
		return nullptr;
	}
}