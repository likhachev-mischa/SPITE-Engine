#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"

// Test Components
struct Position : spite::IComponent
{
	float x, y, z;
	Position() = default;

	Position(float x, float y, float z) : x(x), y(y), z(z)
	{
	}

	bool operator==(const Position&) const = default;
};

struct Velocity : spite::IComponent
{
	float dx, dy, dz;
};

struct Rotator : spite::IComponent
{
	float pitch, yaw, roll;
};

class EcsModificationTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsModificationTestAllocator", 32 * spite::MB)
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
			componentInitializer.registerComponent<Position>();
			componentInitializer.registerComponent<Velocity>();
			componentInitializer.registerComponent<Rotator>();
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


	EcsModificationTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsModificationTest, AddComponentToEntity)
{
	auto entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity, 1.f, 2.f, 3.f);
	ASSERT_TRUE(entityManager.hasComponent<Position>(entity));
	auto& pos = entityManager.getComponent<Position>(entity);
	ASSERT_EQ(pos.x, 1.f);
}

TEST_F(EcsModificationTest, AddMultipleComponentsToEntity)
{
	auto entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity);
	entityManager.addComponent<Velocity>(entity);
	ASSERT_TRUE(entityManager.hasComponent<Position>(entity));
	ASSERT_TRUE(entityManager.hasComponent<Velocity>(entity));
	const auto& aspect = archetypeManager.getEntityAspect(entity);
	ASSERT_EQ(aspect.size(), 2);
}

TEST_F(EcsModificationTest, RemoveComponentFromEntity)
{
	auto entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity);
	entityManager.addComponent<Velocity>(entity);
	entityManager.removeComponent<Position>(entity);
	ASSERT_FALSE(entityManager.hasComponent<Position>(entity));
	ASSERT_TRUE(entityManager.hasComponent<Velocity>(entity));
}

TEST_F(EcsModificationTest, ModifyComponentData)
{
	auto entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity, 1.f, 2.f, 3.f);
	auto& pos = entityManager.getComponent<Position>(entity);
	pos.x = 100.f;
	const auto& cpos = entityManager.getComponent<Position>(entity);
	ASSERT_EQ(cpos.x, 100.f);
}

TEST_F(EcsModificationTest, AddComponentToInvalidEntity)
{
	spite::Entity invalidEntity(999);
	ASSERT_THROW(entityManager.addComponent<Position>(invalidEntity), std::runtime_error);
}

TEST_F(EcsModificationTest, RemoveComponentFromInvalidEntity)
{
	spite::Entity invalidEntity(999);
	ASSERT_THROW(entityManager.removeComponent<Position>(invalidEntity), std::runtime_error);
}

TEST_F(EcsModificationTest, AddExistingComponent)
{
	auto entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity, 1.f, 2.f, 3.f);
	// This should ideally not change the archetype or corrupt data.
	// The current implementation might move the entity to the same archetype.
	ASSERT_NO_THROW(entityManager.addComponent<Position>(entity, 4.f, 5.f, 6.f));
	auto& pos = entityManager.getComponent<Position>(entity);
	// The component's data should be overwritten by the second add.
	ASSERT_EQ(pos.x, 4.f);
}

TEST_F(EcsModificationTest, RemoveNonExistentComponent)
{
	auto entity = entityManager.createEntity();
	// This should not fail, but result in a no-op.
	ASSERT_NO_THROW(entityManager.removeComponent<Position>(entity));
	ASSERT_FALSE(entityManager.hasComponent<Position>(entity));
}

TEST_F(EcsModificationTest, BatchAddComponent)
{
	auto entities = spite::makeHeapVector<spite::Entity>(allocator);
	entityManager.createEntities(10, entities);
	entityManager.addComponents<Position>(entities);
	for (const auto& entity : entities)
	{
		ASSERT_TRUE(entityManager.hasComponent<Position>(entity));
	}
}

TEST_F(EcsModificationTest, BatchRemoveComponent)
{
	auto entities = spite::makeHeapVector<spite::Entity>(allocator);
	spite::Aspect aspect({metadataRegistry.getComponentId(typeid(Position))});
	entityManager.createEntities(10, entities, aspect);
	entityManager.removeComponents<Position>(entities);
	for (const auto& entity : entities)
	{
		ASSERT_FALSE(entityManager.hasComponent<Position>(entity));
	}
}
