#include "ComponentMetadataRegistry.hpp"
#include "ecs/generated/GeneratedComponentRegistration.hpp"
#include "ecs/storage/SharedComponentManager.hpp"

namespace spite
{
	SharedComponentRegistryBridge::SharedComponentRegistryBridge(SharedComponentManager* sharedComponentManager):
		m_sharedComponentManager(sharedComponentManager)
	{
	}

	void SharedComponentRegistryBridge::destroyHandle(SharedComponentHandle handle) const
	{
		m_sharedComponentManager->decrementRef(handle);
	}

	ComponentID ComponentMetadataRegistry::getComponentId(const std::type_index& typeIndex) const
	{
		auto it = m_typeToId.find(typeIndex);
		if (it != m_typeToId.end())
		{
			return it->second;
		}
		return INVALID_COMPONENT_ID;
	}

	const ComponentMetadata& ComponentMetadataRegistry::getMetadata(
		const std::type_index& typeIndex) const
	{
		const ComponentID id = getComponentId(typeIndex);
		SASSERTM(id != INVALID_COMPONENT_ID, "Component of type %s is not registered\n", typeIndex.name())
		return getMetadata(id);
	}

	const ComponentMetadata& ComponentMetadataRegistry::getMetadata(ComponentID id) const
	{
		SASSERTM(id < m_idToMetadata.size(), "Invalid ComponentID %u\n", id)
		return m_idToMetadata[id];
	}

	ComponentMetadataInitializer::ComponentMetadataInitializer(ComponentMetadataRegistry* registry,
	                                                           SharedComponentRegistryBridge* sharedManager):
		m_registry(registry), m_sharedComponentManager(sharedManager)
	{
#ifndef SPITE_TEST
		registerAllComponents(*this);
#endif
	}
}
