#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"

using namespace spite::test;

class EcsQueryTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsQueryTestAllocator", 32 * spite::MB)
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


	EcsQueryTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsQueryTest, SimpleQuery)
{
	auto e1 = entityManager.createEntity();
	entityManager.addComponent<Position>(e1, Position(1, 2, 3));
	auto e2 = entityManager.createEntity();
	entityManager.addComponent<Velocity>(e2, Velocity(4, 5, 6));

	auto query = entityManager.getQueryBuilder().with<Position>().build();
	int count = 0;
	for (auto& pos : query.view<Position>())
	{
		count++;
		ASSERT_EQ(pos.x, 1);
	}
	ASSERT_EQ(count, 1);
}

TEST_F(EcsQueryTest, QueryWithMultipleComponents)
{
	auto e1 = entityManager.createEntity();
	entityManager.addComponent<Position>(e1, Position(1, 2, 3));
	entityManager.addComponent<Velocity>(e1, Velocity(4, 5, 6));

	auto e2 = entityManager.createEntity();
	entityManager.addComponent<Position>(e2, Position(7, 8, 9));

	auto query = entityManager.getQueryBuilder().with<Position, Velocity>().build();
	int count = 0;
	for (auto [pos, vel] : query.view<Position, Velocity>())
	{
		count++;
		ASSERT_EQ(pos.x, 1);
		ASSERT_EQ(vel.dx, 4);
	}
	ASSERT_EQ(count, 1);
}

TEST_F(EcsQueryTest, QueryWithExclusion)
{
	auto e1 = entityManager.createEntity();
	entityManager.addComponent<Position>(e1);
	entityManager.addComponent<TagA>(e1);

	auto e2 = entityManager.createEntity();
	entityManager.addComponent<Position>(e2);

	auto query = entityManager.getQueryBuilder().with<Position>().without<TagA>().build();
	int count = 0;
	for (auto entity : query.view<Position>())
	{
		count++;
	}
	ASSERT_EQ(count, 1);
}

TEST_F(EcsQueryTest, QueryForEntity)
{
	auto e1 = entityManager.createEntity();
	entityManager.addComponent<Position>(e1);

	auto query = entityManager.getQueryBuilder().with<Position>().build();
	for (auto [entity,pos] : query.view<spite::Entity,Position>())
	{
		ASSERT_EQ(entity, e1);
	}
}

TEST_F(EcsQueryTest, QueryAfterStructuralChange)
{
	auto e = entityManager.createEntity();
	entityManager.addComponent<Position>(e);

	auto query = entityManager.getQueryBuilder().with<Position>().build();
	int count1 = 0;
	for (auto it = query.begin<Position>(); it != query.end<Position>(); ++it)
	{
		count1++;
	}
	ASSERT_EQ(count1, 1);

	entityManager.removeComponent<Position>(e);
	// Query should be updated automatically
	auto query2 = entityManager.getQueryBuilder().with<Position>().build();
	int count2 = 0;
	for (auto it = query2.begin<Position>(); it != query2.end<Position>(); ++it)
	{
		count2++;
	}
	ASSERT_EQ(count2, 0);

	entityManager.addComponent<Position>(e);
	auto query3 = entityManager.getQueryBuilder().with<Position>().build();
	int count3 = 0;
	for (auto it = query3.begin<Position>(); it != query3.end<Position>(); ++it)
	{
		count3++;
	}
	ASSERT_EQ(count3, 1);
}

TEST_F(EcsQueryTest, EmptyQuery)
{
	auto query = entityManager.getQueryBuilder().with<Position>().build();
	ASSERT_EQ(query.begin<Position>(), query.end<Position>());
}

TEST_F(EcsQueryTest, QueryWithModificationFilter)
{
	auto e = entityManager.createEntity();
	entityManager.addComponent<Position>(e);

	auto query = entityManager.getQueryBuilder().with<Position>().modified<Position>().build();
	int count = 0;
	for (auto& pos : query.modified_view<Position>())
	{
		count++;
	}
	ASSERT_EQ(count, 1); // Initially modified

	archetypeManager.resetAllModificationTracking();

	count = 0;
	for (auto& pos : query.modified_view<Position>())
	{
		count++;
	}
	ASSERT_EQ(count, 0); // Not modified now

	entityManager.getComponent<Position>(e).x = 5.0f;

	count = 0;
	for (auto& pos : query.modified_view<Position>())
	{
		count++;
	}
	ASSERT_EQ(count, 1); // Modified again
}
