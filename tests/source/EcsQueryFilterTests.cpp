#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "base/memory/HeapAllocator.hpp"

struct ComponentA : spite::IComponent
{
};

struct ComponentB : spite::IComponent
{
};

class EcsQueryFilterTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsQueryFilterTestAllocator", 32 * spite::MB)
		{
		}

		~Allocators() { allocator.shutdown(); }
	};

	struct Container
	{
		spite::ComponentMetadataRegistry metadataRegistry;
		spite::AspectRegistry aspectRegistry;
		spite::VersionManager versionManager;
		spite::ArchetypeManager archetypeManager;
		spite::SharedComponentManager sharedComponentManager;
		spite::SharedComponentRegistryBridge registryBridge;
		spite::EntityManager entityManager;
		spite::ComponentMetadataInitializer componentInitializer;
		spite::ScratchAllocator scratchAllocator;
		spite::QueryRegistry queryRegistry;

		Container(spite::HeapAllocator& allocator) :
			aspectRegistry(allocator)
			, versionManager(allocator, &aspectRegistry)
			, archetypeManager(&metadataRegistry, allocator, &aspectRegistry, &versionManager)
			, sharedComponentManager(metadataRegistry, allocator)
			, queryRegistry(allocator, &archetypeManager, &versionManager, &aspectRegistry, &metadataRegistry)
			, entityManager(archetypeManager, sharedComponentManager, metadataRegistry, &aspectRegistry, &queryRegistry)
			, registryBridge(&sharedComponentManager)
			, componentInitializer(&metadataRegistry, &registryBridge)
			, scratchAllocator(1 * spite::MB)
		{
			componentInitializer.registerComponent<ComponentA>();
			componentInitializer.registerComponent<ComponentB>();
		}
	};

	Allocators* allocContainer = new Allocators;
	Container* container = allocContainer->allocator.new_object<Container>(allocContainer->allocator);

	spite::HeapAllocator& allocator = allocContainer->allocator;
	spite::ComponentMetadataRegistry& metadataRegistry = container->metadataRegistry;
	spite::AspectRegistry& aspectRegistry = container->aspectRegistry;
	spite::VersionManager& versionManager = container->versionManager;
	spite::ArchetypeManager& archetypeManager = container->archetypeManager;
	spite::SharedComponentManager& sharedComponentManager = container->sharedComponentManager;
	spite::QueryRegistry& queryRegistry = container->queryRegistry;
	spite::EntityManager& entityManager = container->entityManager;
	spite::ScratchAllocator& scratchAllocator = container->scratchAllocator;
	spite::ComponentMetadataInitializer& componentInitializer = container->componentInitializer;


	EcsQueryFilterTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsQueryFilterTest, EnabledFilter)
{
	auto e1 = entityManager.createEntity();
	entityManager.addComponent<ComponentA>(e1);
	entityManager.addComponent<ComponentB>(e1);

	auto e2 = entityManager.createEntity();
	entityManager.addComponent<ComponentA>(e2);
	entityManager.addComponent<ComponentB>(e2);

	entityManager.disableComponent<ComponentA>(e1);

	auto query = entityManager.getQueryBuilder().with<ComponentA, ComponentB>().enabled<ComponentA>().build();
	int count = 0;
	for (auto it = query->enabled_begin<ComponentA, ComponentB>(); it != query->enabled_end<ComponentA, ComponentB>();
	     ++it)
	{
		count++;
		ASSERT_EQ(it.getEntity(), e2);
	}
	ASSERT_EQ(count, 1);
}
