#include "TypeInspectorRegistry.hpp"

#include <external/imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "base/Assert.hpp"
#include "base/CollectionUtilities.hpp"
#include "base/Math.hpp"

namespace spite
{
	TypeInspectorRegistry* TypeInspectorRegistry::m_instance = nullptr;

	// Helper for quaternion drawing
	bool InspectQuaternion(const char* name, void* data)
	{
		auto* q = static_cast<glm::quat*>(data);
		glm::vec3 euler = glm::eulerAngles(*q);
		euler = glm::degrees(euler);

		if (ImGui::DragFloat3(name, glm::value_ptr(euler)))
		{
			*q = glm::quat(glm::radians(euler));
			return true;
		}
		return false;
	}

	void TypeInspectorRegistry::init(HeapAllocator& allocator)
	{
		SASSERTM(!m_instance, "TypeInspectorRegistry is already initialized");
		m_instance = allocator.new_object<TypeInspectorRegistry>(allocator);

		// Register default inspectors
		m_instance->registerInspector<bool>([](const char* name, void* data)
		{
			return ImGui::Checkbox(name, static_cast<bool*>(data));
		});

		m_instance->registerInspector<float>([](const char* name, void* data)
		{
			return ImGui::DragFloat(name, static_cast<float*>(data), 0.01f);
		});

		m_instance->registerInspector<glm::vec3>([](const char* name, void* data)
		{
			return ImGui::DragFloat3(name, glm::value_ptr(*static_cast<glm::vec3*>(data)), 0.01f);
		});

		m_instance->registerInspector<glm::quat>(InspectQuaternion);
	}

	void TypeInspectorRegistry::destroy()
	{
		SASSERTM(m_instance, "TypeInspectorRegistry is not initialized");
		m_instance->m_allocator.delete_object(m_instance);
		m_instance = nullptr;
	}

	TypeInspectorRegistry* TypeInspectorRegistry::get()
	{
		SASSERTM(m_instance, "TypeInspectorRegistry is not initialized");
		return m_instance;
	}

	TypeInspectorRegistry::TypeInspectorRegistry(const HeapAllocator& allocator)
		: m_inspectors(makeHeapMap<std::type_index, InspectorFn, std::hash<std::type_index>>(allocator)),
		  m_allocator(allocator)
	{
	}

	bool TypeInspectorRegistry::inspect(const std::type_index& type, const char* name, void* data)
	{
		auto it = m_inspectors.find(type);
		if (it != m_inspectors.end())
		{
			return it->second(name, data);
		}
		return false;
	}
}
