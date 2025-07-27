#include "ComponentMetadataRegistry.hpp"

#include "ecs/storage/SharedComponentManager.hpp"

namespace spite
{
	ComponentMetadataRegistry* ComponentMetadataRegistry::m_instance = nullptr;

	ComponentMetadataRegistry::ComponentMetadataRegistry(const HeapAllocator& allocator)
		: m_idToMetadata(makeHeapVector<ComponentMetadata>(allocator)),
		  m_typeToIdMap(makeHeapMap<std::type_index, ComponentID,std::hash<std::type_index>>(allocator)),m_allocator(allocator)
	{
		// Reserve ID 0 for INVALID_COMPONENT_ID
		m_idToMetadata.emplace_back();
	}

	void DestructionContext::destroySharedHandle(void* component) const
	{
		sharedManager->decrementRef(*static_cast<SharedComponentHandle*>(component));
	}
} 
