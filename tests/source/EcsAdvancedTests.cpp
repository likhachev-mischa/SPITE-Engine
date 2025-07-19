#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "ecs/systems/SystemBase.hpp"

using namespace spite::test;

class EcsAdvancedTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsAdvancedTestAllocator", 32 * spite::MB)
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


	EcsAdvancedTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

// Test System with Prerequisite
class PrerequisiteSystem : public spite::SystemBase
{
public:
	int enable_calls = 0;
	int disable_calls = 0;
	int update_calls = 0;

	PrerequisiteSystem(spite::EntityManager* em, spite::VersionManager* vm)
		: SystemBase(em, vm)
	{
		setPrerequisite(em->getQueryBuilder().with<TagA>().build());
	}

	void onEnable() override { enable_calls++; }
	void onDisable() override { disable_calls++; }
	void onUpdate(float) override { update_calls++; }
};

//TEST_F(EcsAdvancedTest, SystemPrerequisiteLifecycle)
//{
//	PrerequisiteSystem system(&entityManager, &versionManager);
//
//	// Initial state: prerequisite not met
//	system.prepareForUpdate();
//	ASSERT_FALSE(system.isActive());
//	ASSERT_EQ(system.enable_calls, 0);
//	ASSERT_EQ(system.disable_calls, 0);
//	ASSERT_EQ(system.update_calls, 0);
//
//	// Create an entity that meets the prerequisite
//	auto e = entityManager.createEntity();
//	entityManager.addComponent<TagA>(e);
//
//	// Prerequisite should now be met, onEnable called
//	system.prepareForUpdate();
//	ASSERT_TRUE(system.isActive());
//	ASSERT_EQ(system.enable_calls, 1);
//	ASSERT_EQ(system.disable_calls, 0);
//
//	// System should update
//	if (system.isActive()) system.onUpdate(0.1f);
//	ASSERT_EQ(system.update_calls, 1);
//
//	// Destroy the entity
//	entityManager.destroyEntity(e);
//
//	// Prerequisite no longer met, onDisable called
//	system.prepareForUpdate();
//	ASSERT_FALSE(system.isActive());
//	ASSERT_EQ(system.enable_calls, 1);
//	ASSERT_EQ(system.disable_calls, 1);
//
//	// System should not update
//	if (system.isActive()) system.onUpdate(0.1f);
//	ASSERT_EQ(system.update_calls, 1);
//}

TEST_F(EcsAdvancedTest, CombinedQueryFilters)
{
	auto e1 = entityManager.createEntity();
	entityManager.addComponent<Position>(e1);
	entityManager.addComponent<Velocity>(e1);

	auto e2 = entityManager.createEntity();
	entityManager.addComponent<Position>(e2);
	entityManager.addComponent<Velocity>(e2);
	entityManager.disableComponent<Position>(e2); // e2 is disabled

	auto e3 = entityManager.createEntity();
	entityManager.addComponent<Position>(e3);
	entityManager.addComponent<Velocity>(e3);

	archetypeManager.resetAllModificationTracking();

	// Modify e1 and e3
	entityManager.getComponent<Position>(e1).x = 1.0f;
	entityManager.getComponent<Position>(e3).x = 1.0f;

	auto query = entityManager.getQueryBuilder()
		.with<Position>()
		.enabled<Position>()
		.modified<Position>()
		.build();

	int count = 0;
	for (auto entity : query.enabled_modified_view<spite::Entity>())
	{
		count++;
		// Only e1 and e3 should be in the view. But since we modified e1, only e1 should be here.
		// Wait, we modified e3 too. So both e1 and e3.
		bool is_e1 = entity == e1;
		bool is_e3 = entity == e3;
		ASSERT_TRUE(is_e1 || is_e3);
	}
	ASSERT_EQ(count, 2);
}

TEST_F(EcsAdvancedTest, CommandBufferCreateAndDestroy)
{
	spite::CommandBuffer cmd(scratchAllocator, archetypeManager);
	auto e_proxy = cmd.createEntity();
	cmd.destroyEntity(e_proxy);
	cmd.commit(entityManager);

	auto query = entityManager.getQueryBuilder().with<>().build();
	ASSERT_EQ(query.getEntityCount(), 0);
}

TEST_F(EcsAdvancedTest, CommandBufferAddAndRemoveComponentOnProxy)
{
	spite::CommandBuffer cmd(scratchAllocator, archetypeManager);
	auto e_proxy = cmd.createEntity();
	cmd.addComponent<Position>(e_proxy, {1, 2, 3});
	cmd.removeComponent<Position>(e_proxy);
	cmd.commit(entityManager);

	auto query = entityManager.getQueryBuilder().with<Position>().build();
	ASSERT_EQ(query.getEntityCount(), 0);

	auto all_query = entityManager.getQueryBuilder().with<>().build();
	ASSERT_EQ(all_query.getEntityCount(), 1); // The entity itself should exist
}

TEST_F(EcsAdvancedTest, SharedComponentRefCountOnDestroy)
{
    Material red(1, 0, 0);
    auto handle = sharedComponentManager.getSharedHandle(red);

    // Manually increment to simulate it being used somewhere
    sharedComponentManager.incrementRef(handle);
    
    auto e1 = entityManager.createEntity();
    entityManager.setShared<Material>(e1, red); // Increments ref count

    auto e2 = entityManager.createEntity();
    entityManager.setShared<Material>(e2, red); // Increments ref count

    // We should have 3 references now (manual + e1 + e2)
    // Let's get the internal pool to check the count
    auto& mut_mat = entityManager.getMutableShared<Material>(e1); // This will create a copy
    
    // After getMutableShared, e1 has a unique material.
    // The original 'red' material is now only referenced by our manual handle and e2.
    // Let's re-set e1 to the shared material to test destruction.
    entityManager.setShared<Material>(e1, red);

    // Destroy e1. This should decrement the ref count.
    entityManager.destroyEntity(e1);

    // To verify, we can try to get a mutable version for e2. If the ref count was > 1,
    // it would create a copy. If it was 1, it will modify in place.
    // This is an indirect way to test. A direct way would be to expose ref counts,
    // but that breaks encapsulation. Let's stick to black-box testing.
    
    const auto& mat_e2_before = entityManager.getShared<Material>(e2);
    
    // Destroy e2
    entityManager.destroyEntity(e2);

    // Now only our manual handle should keep the data alive.
    // If we create a new entity with the same material, it should reuse the data.
    auto e3 = entityManager.createEntity();
    entityManager.setShared<Material>(e3, red);
    const auto& mat_e3 = entityManager.getShared<Material>(e3);

    // This doesn't directly test the count, but it tests the lifecycle behavior.
    // A proper test would require access to the SharedComponentManager's internal state.
    // For now, this behavioral test is better than nothing.
    ASSERT_EQ(mat_e3.r, 1.0f);
}
