
#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"

using namespace spite::test;

class EcsLifecycleTest : public testing::Test
{
protected:
	int constructor_calls = 0;
	int destructor_calls = 0;
	int move_constructor_calls = 0;

	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsLifecycleTestAllocator", 32 * spite::MB)
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
			                &queryRegistry,allocator),
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


	EcsLifecycleTest()
	{
	}

	void SetUp() override {
		constructor_calls = 0;
		destructor_calls = 0;
		move_constructor_calls = 0;
    }

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsLifecycleTest, ComponentConstructorIsCalledOnAdd)
{
    auto entity = entityManager.createEntity();
    entityManager.addComponent<LifecycleComponent>(entity, &constructor_calls, &destructor_calls, &move_constructor_calls);
    ASSERT_EQ(constructor_calls, 1);
    ASSERT_EQ(destructor_calls, 0);
}

TEST_F(EcsLifecycleTest, ComponentDestructorIsCalledOnRemove)
{
    auto entity = entityManager.createEntity();
    entityManager.addComponent<LifecycleComponent>(entity, &constructor_calls, &destructor_calls, &move_constructor_calls);
    entityManager.removeComponent<LifecycleComponent>(entity);
    ASSERT_EQ(constructor_calls, 1);
    ASSERT_EQ(destructor_calls, 1);
}

TEST_F(EcsLifecycleTest, ComponentDestructorIsCalledOnDestroy)
{
    auto entity = entityManager.createEntity();
    entityManager.addComponent<LifecycleComponent>(entity, &constructor_calls, &destructor_calls, &move_constructor_calls);
    entityManager.destroyEntity(entity);
    ASSERT_EQ(constructor_calls, 1);
    ASSERT_EQ(destructor_calls, 1);
}

TEST_F(EcsLifecycleTest, ComponentIsMovedOnArchetypeChange)
{
    auto entity = entityManager.createEntity();
    entityManager.addComponent<LifecycleComponent>(entity, &constructor_calls, &destructor_calls, &move_constructor_calls);
    ASSERT_EQ(constructor_calls, 1);

    // This will move the entity to a new archetype
    entityManager.addComponent<OtherComponent>(entity);

    // The original component is move-constructed into the new archetype, and then the old one is destructed.
    ASSERT_EQ(move_constructor_calls, 1);
    ASSERT_EQ(destructor_calls, 1);
}
