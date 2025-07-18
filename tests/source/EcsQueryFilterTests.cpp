#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "base/memory/HeapAllocator.hpp"

using namespace spite::test;

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
		spite::AspectRegistry aspectRegistry;
		spite::VersionManager versionManager;
		spite::SharedComponentManager sharedComponentManager;
		spite::ArchetypeManager archetypeManager;
		spite::EntityManager entityManager;
		spite::SingletonComponentRegistry singletonComponentRegistry;
		spite::ScratchAllocator scratchAllocator;
		spite::QueryRegistry queryRegistry;

		Container(spite::HeapAllocator& allocator) :
			aspectRegistry(allocator)
			, versionManager(allocator, &aspectRegistry)
			, sharedComponentManager(allocator)
			, archetypeManager(allocator, &aspectRegistry, &versionManager, &sharedComponentManager)
			, entityManager(&archetypeManager, &sharedComponentManager, &singletonComponentRegistry, &aspectRegistry,
			                &queryRegistry),
			singletonComponentRegistry(),
			scratchAllocator(1 * spite::MB)
			, queryRegistry(allocator, &archetypeManager, &versionManager)
		{
		}
	};


	Allocators* allocContainer = new Allocators;
	Container* container = allocContainer->allocator.new_object<Container>(allocContainer->allocator);

	spite::HeapAllocator& allocator = allocContainer->allocator;
	spite::AspectRegistry& aspectRegistry = container->aspectRegistry;
	spite::VersionManager& versionManager = container->versionManager;
	spite::ArchetypeManager& archetypeManager = container->archetypeManager;
	spite::SharedComponentManager& sharedComponentManager = container->sharedComponentManager;
	spite::QueryRegistry& queryRegistry = container->queryRegistry;
	spite::EntityManager& entityManager = container->entityManager;
	spite::ScratchAllocator& scratchAllocator = container->scratchAllocator;


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
	for (auto [a,b,entity] : query.enabled_view<ComponentA, ComponentB, spite::Entity>())
	{
		count++;
		ASSERT_EQ(entity, e2);
	}
	ASSERT_EQ(count, 1);
}
