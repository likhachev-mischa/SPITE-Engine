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

class EcsCoreTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators(): allocator("EcsCoreTestAllocator", 32 * spite::MB)
		{
			//spite::initGlobalAllocator();
			//spite::FrameScratchAllocator::init();
		}

		~Allocators()
		{
			allocator.shutdown();
			//spite::FrameScratchAllocator::shutdown();
			//spite::shutdownGlobalAllocator();
		}
	};

	Allocators* allocStorage = new Allocators;

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

	spite::HeapAllocator& allocator = allocStorage->allocator;

	Container* container = allocator.new_object<Container>(allocator);

	spite::AspectRegistry& aspectRegistry = container->aspectRegistry;
	spite::VersionManager& versionManager = container->versionManager;
	spite::ArchetypeManager& archetypeManager = container->archetypeManager;
	spite::SharedComponentManager& sharedComponentManager = container->sharedComponentManager;
	spite::EntityManager& entityManager = container->entityManager;


	EcsCoreTest()
	{
		spite::ComponentMetadataRegistry::registerComponent<Position>();
		spite::ComponentMetadataRegistry::registerComponent<Velocity>();
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocStorage;
	}
};

TEST_F(EcsCoreTest, EntityCreationAndDestruction)
{
	spite::Entity entity = entityManager.createEntity();
	ASSERT_NE(entity.id(), 0);
	ASSERT_TRUE(archetypeManager.isEntityTracked(entity));

	entityManager.destroyEntity(entity);
	ASSERT_FALSE(archetypeManager.isEntityTracked(entity));
}

TEST_F(EcsCoreTest, AddAndHasComponent)
{
	spite::Entity entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity, 1.0f, 2.0f, 3.0f);

	ASSERT_TRUE(entityManager.hasComponent<Position>(entity));
	ASSERT_FALSE(entityManager.hasComponent<Velocity>(entity));
}

TEST_F(EcsCoreTest, GetComponent)
{
	spite::Entity entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity, 1.0f, 2.0f, 3.0f);

	const auto& pos = entityManager.getComponent<Position>(entity);
	ASSERT_EQ(pos.x, 1.0f);
	ASSERT_EQ(pos.y, 2.0f);
	ASSERT_EQ(pos.z, 3.0f);
}

TEST_F(EcsCoreTest, ModifyComponent)
{
	spite::Entity entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity, 1.0f, 2.0f, 3.0f);

	auto& pos = entityManager.getComponent<Position>(entity);
	pos.x = 4.0f;

	const auto& newPos = entityManager.getComponent<Position>(entity);
	ASSERT_EQ(newPos.x, 4.0f);
}

TEST_F(EcsCoreTest, RemoveComponent)
{
	spite::Entity entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity);
	ASSERT_TRUE(entityManager.hasComponent<Position>(entity));

	entityManager.removeComponent<Position>(entity);
	ASSERT_FALSE(entityManager.hasComponent<Position>(entity));
}

TEST_F(EcsCoreTest, ArchetypeMovement)
{
	spite::Entity entity = entityManager.createEntity();
	const auto& initialAspect = archetypeManager.getEntityAspect(entity);
	ASSERT_EQ(initialAspect.size(), 0);

	entityManager.addComponent<Position>(entity);
	const auto& aspectWithPos = archetypeManager.getEntityAspect(entity);
	ASSERT_EQ(aspectWithPos.size(), 1);
	ASSERT_TRUE(aspectWithPos.contains(spite::ComponentMetadataRegistry::getComponentId<Position>()));

	entityManager.addComponent<Velocity>(entity);
	const auto& aspectWithPosVel = archetypeManager.getEntityAspect(entity);
	ASSERT_EQ(aspectWithPosVel.size(), 2);
	ASSERT_TRUE(aspectWithPosVel.contains(spite::ComponentMetadataRegistry::getComponentId<Position>()));
	ASSERT_TRUE(aspectWithPosVel.contains(spite::ComponentMetadataRegistry::getComponentId<Velocity>()));

	entityManager.removeComponent<Position>(entity);
	const auto& aspectWithVel = archetypeManager.getEntityAspect(entity);
	ASSERT_EQ(aspectWithVel.size(), 1);
	ASSERT_TRUE(aspectWithVel.contains(spite::ComponentMetadataRegistry::getComponentId<Velocity>()));
	ASSERT_FALSE(aspectWithVel.contains(spite::ComponentMetadataRegistry::getComponentId<Position>()));
}

TEST_F(EcsCoreTest, InvalidEntity)
{
	spite::Entity invalidEntity(999);
	ASSERT_THROW(entityManager.getComponent<Position>(invalidEntity), std::runtime_error);
	ASSERT_THROW(entityManager.addComponent<Position>(invalidEntity), std::runtime_error);
	ASSERT_THROW(entityManager.removeComponent<Position>(invalidEntity), std::runtime_error);
	ASSERT_THROW(entityManager.destroyEntity(invalidEntity), std::runtime_error);
}

TEST_F(EcsCoreTest, CreateMultipleEntities)
{
	const size_t count = 10;
	auto entities(spite::makeHeapVector<spite::Entity>(allocator));
	entityManager.createEntities(count, entities);

	ASSERT_EQ(entities.size(), count);
	for (const auto& entity : entities)
	{
		ASSERT_TRUE(archetypeManager.isEntityTracked(entity));
	}
}
