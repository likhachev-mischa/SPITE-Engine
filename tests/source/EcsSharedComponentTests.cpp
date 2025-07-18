#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"

using namespace spite::test;

// Test Shared Components

class EcsSharedComponentTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsSharedComponentTestAllocator", 32 * spite::MB)
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


	EcsSharedComponentTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsSharedComponentTest, SetAndGetSharedComponent)
{
	auto e = entityManager.createEntity();
	entityManager.setShared<Material>(e,Material(1.0f, 0.0f, 0.0f));
	ASSERT_TRUE(entityManager.hasComponent<spite::SharedComponent<Material>>(e));
	const auto& mat = entityManager.getShared<Material>(e);
	ASSERT_EQ(mat.r, 1.0f);
}

TEST_F(EcsSharedComponentTest, MultipleEntitiesShareComponent)
{
	auto e1 = entityManager.createEntity();
	auto e2 = entityManager.createEntity();
	entityManager.setShared<Material>(e1, Material(0.5f, 0.5f, 0.5f));
	entityManager.setShared<Material>(e2, Material(0.5f, 0.5f, 0.5f));

	const auto& mat1 = entityManager.getShared<Material>(e1);
	const auto& mat2 = entityManager.getShared<Material>(e2);
	ASSERT_EQ(&mat1, &mat2); // Should point to the same data
}

TEST_F(EcsSharedComponentTest, ModifySharedComponent)
{
	auto e1 = entityManager.createEntity();
	auto e2 = entityManager.createEntity();
	entityManager.setShared<Material>(e1, Material(0.5f, 0.5f, 0.5f));
	entityManager.setShared<Material>(e2, Material(0.5f, 0.5f, 0.5f));

	// This should perform a copy-on-write
	auto& mat1_mut = entityManager.getMutableShared<Material>(e1);
	mat1_mut.r = 1.0f;

	const auto& mat1_after = entityManager.getShared<Material>(e1);
	const auto& mat2_after = entityManager.getShared<Material>(e2);

	ASSERT_NE(&mat1_after, &mat2_after);
	ASSERT_EQ(mat1_after.r, 1.0f);
	ASSERT_EQ(mat2_after.r, 0.5f);
}
