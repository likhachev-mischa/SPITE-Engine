
#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"

// Test Components
struct Transform : spite::IComponent { float x, y, z; };
struct Renderable : spite::IComponent { int meshId; };

class EcsCreationTest : public testing::Test {
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsCreationTestAllocator", 32 * spite::MB)
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
			componentInitializer.registerComponent<Transform>();
			componentInitializer.registerComponent<Renderable>();
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


	EcsCreationTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsCreationTest, CreateSingleEntity) {
    spite::Entity entity = entityManager.createEntity();
    ASSERT_NE(entity.id(), 0);
    ASSERT_TRUE(archetypeManager.isEntityTracked(entity));
    const auto& aspect = archetypeManager.getEntityAspect(entity);
    ASSERT_TRUE(aspect.empty());
}

TEST_F(EcsCreationTest, CreateSingleEntityWithComponents) {
    spite::Aspect aspect({metadataRegistry.getComponentId(typeid(Transform))});
    spite::Entity entity = entityManager.createEntity(aspect);
    ASSERT_TRUE(entityManager.hasComponent<Transform>(entity));
    ASSERT_FALSE(entityManager.hasComponent<Renderable>(entity));
}

TEST_F(EcsCreationTest, CreateMultipleEntities) {
    const size_t count = 100;
    auto entities = spite::makeHeapVector<spite::Entity>(allocator);
    entityManager.createEntities(count, entities);
    ASSERT_EQ(entities.size(), count);
    for (const auto& entity : entities) {
        ASSERT_TRUE(archetypeManager.isEntityTracked(entity));
    }
}

TEST_F(EcsCreationTest, CreateMultipleEntitiesWithComponents) {
    const size_t count = 50;
    spite::Aspect aspect({metadataRegistry.getComponentId(typeid(Transform)), metadataRegistry.getComponentId(typeid(Renderable))});
    auto entities = spite::makeHeapVector<spite::Entity>(allocator);
    entityManager.createEntities(count, entities, aspect);
    ASSERT_EQ(entities.size(), count);
    for (const auto& entity : entities) {
        ASSERT_TRUE(entityManager.hasComponent<Transform>(entity));
        ASSERT_TRUE(entityManager.hasComponent<Renderable>(entity));
    }
}

TEST_F(EcsCreationTest, DestroySingleEntity) {
    spite::Entity entity = entityManager.createEntity();
    entityManager.destroyEntity(entity);
    ASSERT_FALSE(archetypeManager.isEntityTracked(entity));
}

TEST_F(EcsCreationTest, DestroyMultipleEntities) {
    const size_t count = 100;
    auto entities = spite::makeHeapVector<spite::Entity>(allocator);
    entityManager.createEntities(count, entities);
    entityManager.destroyEntities(entities);
    for (const auto& entity : entities) {
        ASSERT_FALSE(archetypeManager.isEntityTracked(entity));
    }
}

TEST_F(EcsCreationTest, CreateAndDestroyInterleaved) {
    auto e1 = entityManager.createEntity();
    auto e2 = entityManager.createEntity();
    entityManager.destroyEntity(e1);
    auto e3 = entityManager.createEntity();
    ASSERT_FALSE(archetypeManager.isEntityTracked(e1));
    ASSERT_TRUE(archetypeManager.isEntityTracked(e2));
    ASSERT_TRUE(archetypeManager.isEntityTracked(e3));
}
