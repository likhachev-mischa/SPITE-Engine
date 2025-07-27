#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"


struct Position : spite::IComponent
{
	float x, y, z;
	Position() = default;

	Position(float x, float y, float z) : x(x), y(y), z(z)
	{
	}
};

struct Velocity : spite::IComponent
{
	float dx, dy, dz;

	Velocity() = default;

	Velocity(float x, float y, float z) : dx(x), dy(y), dz(z)
	{
	}
};

struct TagA : spite::IComponent
{
};

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
			singletonComponentRegistry(allocator),
			scratchAllocator(1 * spite::MB)
			, queryRegistry(allocator, &archetypeManager, &versionManager)
		{
		}

		~Container()
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
		spite::ComponentMetadataRegistry::registerComponent<Position>();
		spite::ComponentMetadataRegistry::registerComponent<Velocity>();
		spite::ComponentMetadataRegistry::registerComponent<TagA>();
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

	auto query = entityManager.getQueryBuilder().with_read<Position>().build();
	int count = 0;
	for (auto& pos : query.view<spite::Read<Position>>())
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

	auto query = entityManager.getQueryBuilder().with_read<Position, Velocity>().build();
	int count = 0;
	for (auto [pos, vel] : query.view<spite::Read<Position>, spite::Read<Velocity>>())
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

	auto query = entityManager.getQueryBuilder().with_read<Position>().without<TagA>().build();
	int count = 0;
	for (auto entity : query.view<spite::Read<Position>>())
	{
		count++;
	}
	ASSERT_EQ(count, 1);
}

TEST_F(EcsQueryTest, QueryForEntity)
{
	auto e1 = entityManager.createEntity();
	entityManager.addComponent<Position>(e1);

	auto query = entityManager.getQueryBuilder().with_read<Position>().build();
	for (auto [entity,pos] : query.view<spite::Entity,spite::Read<Position>>())
	{
		ASSERT_EQ(entity, e1);
	}
}

TEST_F(EcsQueryTest, QueryAfterStructuralChange)
{
	auto e = entityManager.createEntity();
	entityManager.addComponent<Position>(e);

	auto query = entityManager.getQueryBuilder().with_read<Position>().build();
	int count1 = 0;
	for (auto pos : query.view<spite::Read<Position>>())
	{
		count1++;
	}
	ASSERT_EQ(count1, 1);

	entityManager.removeComponent<Position>(e);
	// Query should be updated automatically
	auto query2 = entityManager.getQueryBuilder().with_read<Position>().build();
	int count2 = 0;
	for (auto pos : query2.view<spite::Read<Position>>())
	{
		count2++;
	}
	ASSERT_EQ(count2, 0);

	entityManager.addComponent<Position>(e);
	auto query3 = entityManager.getQueryBuilder().with_read<Position>().build();
	int count3 = 0;
	for (auto pos : query3.view<spite::Read<Position>>())
	{
		count3++;
	}
	ASSERT_EQ(count3, 1);
}

TEST_F(EcsQueryTest, EmptyQuery)
{
	auto query = entityManager.getQueryBuilder().with_read<Position>().build();
	ASSERT_EQ(query.begin<spite::Read<Position>>(), query.end<spite::Read<Position>>());
}

TEST_F(EcsQueryTest, QueryWithModificationFilter)
{
	auto e = entityManager.createEntity();
	entityManager.addComponent<Position>(e);

	auto query = entityManager.getQueryBuilder().with_read<Position>().modified<Position>().build();
	int count = 0;
	for (auto& pos : query.view<spite::Read<Position>>())
	{
		count++;
	}
	ASSERT_EQ(count, 1); // Initially modified

	archetypeManager.resetAllModificationTracking();

	count = 0;
	for (auto& pos : query.view<spite::Read<Position>>())
	{
		count++;
	}
	ASSERT_EQ(count, 0); // Not modified now

	entityManager.getComponent<Position>(e).x = 5.0f;

	count = 0;
	for (auto& pos : query.view<spite::Read<Position>>())
	{
		count++;
	}
	ASSERT_EQ(count, 1); // Modified again
}
