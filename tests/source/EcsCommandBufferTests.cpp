#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "base/memory/ScratchAllocator.hpp"
#include "ecs/config/TestComponents.hpp"

using namespace spite::test;
class EcsCommandBufferTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsCommandBufferTestAllocator", 32 * spite::MB)
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

	EcsCommandBufferTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsCommandBufferTest, CreateEntity)
{
	spite::CommandBuffer cmd(scratchAllocator, archetypeManager);
	auto e_proxy = cmd.createEntity();
	cmd.commit(entityManager);

	auto query = entityManager.getQueryBuilder().with<>().build();

	ASSERT_EQ(query.getEntityCount(), 1);
}

TEST_F(EcsCommandBufferTest, DestroyEntity)
{
	auto e = entityManager.createEntity();
	ASSERT_TRUE(archetypeManager.isEntityTracked(e));

	spite::CommandBuffer cmd(scratchAllocator, archetypeManager);
	cmd.destroyEntity(e);
	cmd.commit(entityManager);

	ASSERT_FALSE(archetypeManager.isEntityTracked(e));
}

TEST_F(EcsCommandBufferTest, AddComponent)
{
	spite::CommandBuffer cmd(scratchAllocator, archetypeManager);
	auto e_proxy = cmd.createEntity();
	cmd.addComponent<Position>(e_proxy, {1, 2, 3});
	cmd.commit(entityManager);

	auto query = entityManager.getQueryBuilder().with<Position>().build();
	int count = 0;
	spite::Entity realEntity;
	for (auto it = query.begin<Position>(); it != query.end<Position>(); ++it)
	{
		count++;
		realEntity = it.getEntity();
	}
	ASSERT_EQ(count, 1);
	ASSERT_TRUE(entityManager.hasComponent<Position>(realEntity));
	const auto& pos = entityManager.getComponent<Position>(realEntity);
	ASSERT_EQ(pos.x, 1);
}

TEST_F(EcsCommandBufferTest, RemoveComponent)
{
	auto e = entityManager.createEntity();
	entityManager.addComponent<Position>(e);

	spite::CommandBuffer cmd(scratchAllocator, archetypeManager);
	cmd.removeComponent<Position>(e);
	cmd.commit(entityManager);

	ASSERT_FALSE(entityManager.hasComponent<Position>(e));
}

TEST_F(EcsCommandBufferTest, MixedCommands)
{
	spite::CommandBuffer cmd(scratchAllocator, archetypeManager);

	auto e1_proxy = cmd.createEntity();
	cmd.addComponent<Position>(e1_proxy, {1, 1, 1});

	auto e2 = entityManager.createEntity();
	entityManager.addComponent<Velocity>(e2);
	ASSERT_TRUE(archetypeManager.isEntityTracked(e2));
	cmd.addComponent<Position>(e2, {2, 2, 2});
	cmd.destroyEntity(e2);

	cmd.commit(entityManager);

	auto query = entityManager.getQueryBuilder().with<Position>().build();
	int count = 0;
	for (auto it = query.begin<Position>(); it != query.end<Position>(); ++it)
	{
		count++;
	}
	ASSERT_EQ(count, 1);
	ASSERT_FALSE(archetypeManager.isEntityTracked(e2));
}
