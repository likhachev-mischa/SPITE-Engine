#include "ComponentMetadataRegistry.hpp"

#include "ecs/storage/SharedComponentManager.hpp"

namespace spite
{
	// Define the static members
	eastl::array<ComponentMetadata, ComponentMetadataRegistry::MAX_COMPONENTS> ComponentMetadataRegistry::m_idToMetadata;
	std::once_flag ComponentMetadataRegistry::m_onceFlag;

	void ComponentMetadataRegistry::initialize()
	{
		// Set the default metadata for ID 0
		m_idToMetadata[0] = ComponentMetadata{};

		// Use a helper to iterate the ComponentList tuple and populate the metadata array.
		auto populate_metadata = [&]<typename T>()
		{
			constexpr ComponentID id = ComponentMetadataRegistry::getComponentId<T>();
			m_idToMetadata[id] = detail::create_metadata_for<T>();
		};

		detail::for_each_in_tuple<ComponentList>(populate_metadata);
	}

	void DestructionContext::destroySharedHandle(void* component) const
	{
		sharedManager->decrementRef(*static_cast<SharedComponentHandle*>(component));
	}
} 
