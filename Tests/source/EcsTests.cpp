#if !defined(SPITE_TEST)
#define SPITE_TEST
#endif

#include <base/Logging.hpp>
#include <base/Assert.hpp>
#include <base/LoggingTestStrings.hpp>

#include <ecs/Core.hpp>
#include <ecs/World.hpp>

#include <gtest/gtest.h>

#include "EcsTests.hpp"

namespace
{
	struct PositionComponent : spite::IComponent
	{
		float x, y, z;

		PositionComponent(): IComponent()
		{
		}

		PositionComponent(float x, float y, float z): IComponent(), x(x), y(y), z(z)
		{
		}
	};

	struct VelocityComponent : spite::IComponent
	{
		float x, y, z;

		VelocityComponent(): IComponent()
		{
		}

		VelocityComponent(float x, float y, float z): IComponent(), x(x), y(y), z(z)
		{
		}
	};
}

class EcsTest : public testing::Test
{
protected:
	EcsTest() : m_allocator("ECS TEST ALLOCATOR")
	{
		m_world = new spite::EntityWorld(m_allocator, m_allocator);
	}

	void TearDown() override
	{
		delete m_world;
		m_allocator.shutdown();
		spite::getTestLoggerInstance().dispose();
	}

	spite::HeapAllocator m_allocator;
	spite::EntityWorld* m_world;
};

TEST_F(EcsTest, EntityCreation)
{
	auto entity = m_world->service()->entityManager()->createEntity();
	EXPECT_NE(entity.id(), 0);

	auto entity2 = m_world->service()->entityManager()->createEntity();
	EXPECT_NE(entity.id(), entity2.id());
}

TEST_F(EcsTest, EntityComparison)
{
	auto entity1 = m_world->service()->entityManager()->createEntity();
	auto entity2 = m_world->service()->entityManager()->createEntity();
	auto entity1Copy = entity1;

	EXPECT_EQ(entity1, entity1Copy);
	EXPECT_NE(entity1, entity2);
}

TEST_F(EcsTest, ComponentLifecycle)
{
	// Test component addition, removal, and cleanup
	auto entity = m_world->service()->entityManager()->createEntity();
	auto componentManager = m_world->service()->componentManager();

	// Add component
	componentManager->addComponent<test1::Component>(entity);
	EXPECT_TRUE(componentManager->hasComponent<test1::Component>(entity));
	EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(), 1);

	// Remove component
	componentManager->removeComponent<test1::Component>(entity);
	EXPECT_FALSE(componentManager->hasComponent<test1::Component>(entity));
	EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(), 0);
}

TEST_F(EcsTest, MultiComponentLifecycle)
{
	const sizet entitesCount = 10;
	spite::Entity entities[entitesCount];

	auto entityManager = m_world->service()->entityManager();
	auto componentManager = m_world->service()->componentManager();

	for (sizet i = 0; i < entitesCount; ++i)
	{
		entities[i] = entityManager->createEntity();
		componentManager->addComponent<test1::Component>(entities[i]);
		EXPECT_TRUE(componentManager->hasComponent<test1::Component>(entities[i]));
		componentManager->getComponent<test1::Component>(entities[i]).data = i;

		EXPECT_EQ(componentManager->getComponent<test1::Component>(entities[i]).data, i);

		EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(),
		          i + 1);
	}

	for (sizet i = 0; i < entitesCount - 1; ++i)
	{
		componentManager->removeComponent<test1::Component>(entities[i]);
		EXPECT_FALSE(componentManager->hasComponent<test1::Component>(entities[i]));

		EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(),
		          entitesCount - i - 1);
	}

	EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(),
	          1);

	EXPECT_TRUE(componentManager->hasComponent<test1::Component>(entities[entitesCount - 1]));
	EXPECT_EQ(componentManager->getComponent<test1::Component>(entities[entitesCount - 1]).data, entitesCount - 1);
	componentManager->removeComponent<test1::Component>(entities[entitesCount - 1]);

	EXPECT_FALSE(componentManager->hasComponent<test1::Component>(entities[entitesCount - 1]));
	EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(),
	          0);
}

//checks component cleanup and operations invalidation for deleted entity
TEST_F(EcsTest, EntityDeletion)
{
	// Test component addition, removal, and cleanup
	auto entity = m_world->service()->entityManager()->createEntity();
	auto componentManager = m_world->service()->componentManager();

	// Add component
	componentManager->addComponent<test1::Component>(entity);
	EXPECT_TRUE(componentManager->hasComponent<test1::Component>(entity));
	EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(), 1);

	m_world->service()->entityManager()->deleteEntity(entity);
	EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(), 0);

	EXPECT_ANY_THROW(m_world->service()->entityManager()->deleteEntity(entity));
	EXPECT_ANY_THROW(m_world->service()->componentManager()->removeComponent<test1::Component>(entity));
	EXPECT_ANY_THROW(m_world->service()->componentLookup()->untrackEntity(entity));
}

TEST_F(EcsTest, CommandBuffer)
{
	sizet entitesCount = 10;
	auto* entities = new spite::Entity[entitesCount];

	auto entityManager = m_world->service()->entityManager();
	auto componentManager = m_world->service()->componentManager();
	m_world->service()->componentStorage()->registerComponent<test1::Component>();

	auto additionCbuf = m_world->service()->getCommandBuffer<test1::Component>();
	additionCbuf.reserveForAddition(entitesCount);

	for (sizet i = 0; i < entitesCount; ++i)
	{
		entities[i] = entityManager->createEntity();
		test1::Component component;
		additionCbuf.addComponent(entities[i], component);
		EXPECT_FALSE(componentManager->hasComponent<test1::Component>(entities[i]));

		EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(),
		          0);
	}

	additionCbuf.commit();
	EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(),
	          entitesCount);

	auto removalCbuf = m_world->service()->getCommandBuffer<test1::Component>();
	removalCbuf.reserveForRemoval(entitesCount);
	for (sizet i = 0; i < entitesCount; ++i)
	{
		EXPECT_TRUE(componentManager->hasComponent<test1::Component>(entities[i]));
		removalCbuf.removeComponent(entities[i]);

		EXPECT_TRUE(componentManager->hasComponent<test1::Component>(entities[i]));
		EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(),
		          entitesCount);
	}

	removalCbuf.commit();
	EXPECT_EQ(m_world->service()->componentStorage()->getComponentsAsserted<test1::Component>().getOccupiedSize(),
	          0);
	for (sizet i = 0; i < entitesCount; ++i)
	{
		EXPECT_FALSE(componentManager->hasComponent<test1::Component>(entities[i]));
	}
}

//creates set amount of entities once, writes to their components and then checks the components' value
TEST_F(EcsTest, BasicWriteRead)
{
	spite::SystemBase* sys1 = new test1::CreateComponentSystem;
	spite::SystemBase* sys2 = new test1::WriteComponentSystem;
	spite::SystemBase* sys3 = new test1::ReadComponentSystem;

	m_world->addSystem(sys1);
	m_world->addSystem(sys2);
	m_world->addSystem(sys3);

	m_world->start();
	m_world->enable();

	sizet iterationsCount = 100;
	for (sizet i = 0; i < iterationsCount; ++i)
	{
		m_world->update(0.01f);
		m_world->commitSystemsStructuralChange();
	}

	sizet structrualChangesCount = spite::getTestLogCount(
		TESTLOG_ECS_STRUCTURAL_CHANGE_HAPPENED(typeid(test1::Component).name()));

	EXPECT_EQ(structrualChangesCount, test1::CREATED_ENTITIES_COUNT);
}

//each update creates an entity, writes to it's component and then checks the component's value
TEST_F(EcsTest, BasicCreateWriteRead)
{
	spite::SystemBase* sys1 = new test2::CreateComponentSystem;
	spite::SystemBase* sys2 = new test2::WriteComponentSystem;
	spite::SystemBase* sys3 = new test2::ReadComponentSystem;

	m_world->addSystem(sys1);
	m_world->addSystem(sys2);
	m_world->addSystem(sys3);

	m_world->start();
	m_world->enable();

	sizet iterationsCount = 100;
	for (sizet i = 0; i < iterationsCount; ++i)
	{
		m_world->update(0.01f);
		m_world->commitSystemsStructuralChange();
	}

	sizet structrualChangesCount = spite::getTestLogCount(
		TESTLOG_ECS_STRUCTURAL_CHANGE_HAPPENED(typeid(test2::Component).name()));

	EXPECT_EQ(structrualChangesCount, iterationsCount);
}

TEST_F(EcsTest, AddAndGetComponent)
{
	spite::Entity entity = m_world->service()->entityManager()->createEntity();
	auto componentManager = m_world->service()->componentManager();

	// Add a component
	PositionComponent pos = {1.0f, 2.0f, 3.0f};
	componentManager->addComponent(entity, pos);

	// Get the component
	PositionComponent* retrievedPos = &componentManager->getComponent<PositionComponent>(entity);
	ASSERT_NE(retrievedPos, nullptr);
	ASSERT_EQ(retrievedPos->x, 1.0f);
	ASSERT_EQ(retrievedPos->y, 2.0f);
	ASSERT_EQ(retrievedPos->z, 3.0f);
}

TEST_F(EcsTest, RemoveComponent)
{
	spite::Entity entity = m_world->service()->entityManager()->createEntity();
	auto componentManager = m_world->service()->componentManager();

	// Add a component
	PositionComponent pos = {1.0f, 2.0f, 3.0f};
	componentManager->addComponent(entity, pos);

	// Remove the component
	componentManager->removeComponent<PositionComponent>(entity);

	// Try to get the component, should throw
	ASSERT_ANY_THROW(componentManager->getComponent<PositionComponent>(entity));
}

TEST_F(EcsTest, HasComponent)
{
	spite::Entity entity = m_world->service()->entityManager()->createEntity();
	auto componentManager = m_world->service()->componentManager();

	// Initially, entity shouldn't have the component
	ASSERT_FALSE(componentManager->hasComponent<PositionComponent>(entity));

	// Add the component
	PositionComponent pos = {1.0f, 2.0f, 3.0f};
	componentManager->addComponent(entity, pos);

	// Now, the entity should have the component
	ASSERT_TRUE(componentManager->hasComponent<PositionComponent>(entity));

	// Remove the component
	componentManager->removeComponent<PositionComponent>(entity);

	// Finally, the entity shouldn't have the component
	ASSERT_FALSE(componentManager->hasComponent<PositionComponent>(entity));
}

TEST_F(EcsTest, MultipleComponents)
{
	spite::Entity entity = m_world->service()->entityManager()->createEntity();
	auto componentManager = m_world->service()->componentManager();

	// Add Position
	PositionComponent pos = {1.0f, 2.0f, 3.0f};
	componentManager->addComponent(entity, pos);

	// Add Velocity
	VelocityComponent vel = {4.0f, 5.0f, 6.0f};
	componentManager->addComponent(entity, vel);

	// Check if both components are present
	ASSERT_TRUE(componentManager->hasComponent<PositionComponent>(entity));
	ASSERT_TRUE(componentManager->hasComponent<VelocityComponent>(entity));

	// Retrieve and verify values
	PositionComponent* retrievedPos = &componentManager->getComponent<PositionComponent>(entity);
	VelocityComponent* retrievedVel = &componentManager->getComponent<VelocityComponent>(entity);

	ASSERT_NE(retrievedPos, nullptr);
	ASSERT_NE(retrievedVel, nullptr);

	ASSERT_EQ(retrievedPos->x, 1.0f);
	ASSERT_EQ(retrievedVel->y, 5.0f);
}

TEST_F(EcsTest, SystemAddAndRemove)
{
	static int initCount = 0;
	static int startCount = 0;
	static int updateCount = 0;
	static int destroyCount = 0;
	class TestSystem : public spite::SystemBase
	{
	public:
		void onInitialize() override
		{
			initCount++;
		}

		void onStart() override
		{
			startCount++;
		}

		void onUpdate(float deltaTime) override
		{
			updateCount++;
		}

		void onDestroy() override
		{
			destroyCount++;
		}
	};

	TestSystem* system = new TestSystem();
	m_world->addSystem(system);

	ASSERT_EQ(initCount, 1);
	ASSERT_EQ(startCount, 0);
	ASSERT_EQ(updateCount, 0);
	ASSERT_EQ(destroyCount, 0);

	m_world->start();
	ASSERT_EQ(startCount, 1);

	m_world->update(0.1f);
	ASSERT_EQ(updateCount, 1);

	m_world->destroySystem(system);
	ASSERT_EQ(destroyCount, 1);
}

TEST_F(EcsTest, StructuralChangeHandler)
{
	auto entity = m_world->service()->entityManager()->createEntity();
	auto componentManager = m_world->service()->componentManager();
	auto tracker = m_world->service()->structuralChangeTracker();

	// Initially, the tracker should have no empty or non-empty tables
	ASSERT_EQ(tracker->getEmptyTables().size(), 0);
	ASSERT_EQ(tracker->getNonEmptyTables().size(), 0);

	// Add a component
	componentManager->addComponent<PositionComponent>(entity);

	// Now, the tracker should have one non-empty table
	ASSERT_EQ(tracker->getEmptyTables().size(), 0);
	ASSERT_EQ(tracker->getNonEmptyTables().size(), 1);

	// Remove the component
	componentManager->removeComponent<PositionComponent>(entity);

	// Finally, the tracker should have one empty table
	ASSERT_EQ(tracker->getEmptyTables().size(), 1);
	ASSERT_EQ(tracker->getNonEmptyTables().size(), 0);
}

TEST_F(EcsTest, SystemDependency)
{
	class DependentSystem : public spite::SystemBase
	{
	public:
		bool canUpdate = false;

		void onInitialize() override
		{
			requireComponent(typeid(PositionComponent));
		}

		void onUpdate(float deltaTime) override
		{
			canUpdate = true;
		}
	};

	DependentSystem* dependentSystem = new DependentSystem();
	m_world->addSystem(dependentSystem);
	m_world->start();
	m_world->enable();

	// Initially, the system shouldn't update because there are no PositionComponents
	m_world->update(0.1f);
	ASSERT_FALSE(dependentSystem->canUpdate);

	// Create an entity and add a PositionComponent
	auto entity = m_world->service()->entityManager()->createEntity();
	auto componentManager = m_world->service()->componentManager();
	componentManager->addComponent<PositionComponent>(entity);
	m_world->commitSystemsStructuralChange();

	// Now, the system should update
	m_world->update(0.1f);
	ASSERT_TRUE(dependentSystem->canUpdate);
}

TEST_F(EcsTest, CommandBufferMultipleAdds)
{
	auto entityManager = m_world->service()->entityManager();
	auto componentManager = m_world->service()->componentManager();

	const size_t entityCount = 5;
	spite::Entity entities[entityCount];

	// Create entities
	for (size_t i = 0; i < entityCount; ++i)
	{
		entities[i] = entityManager->createEntity();
	}

	// Create a command buffer for adding PositionComponent
	auto commandBuffer = m_world->service()->getCommandBuffer<PositionComponent>();
	commandBuffer.reserveForAddition(entityCount);

	m_world->service()->componentStorage()->registerComponent<PositionComponent>();

	// Add components to the command buffer
	for (size_t i = 0; i < entityCount; ++i)
	{
		PositionComponent pos = {static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3)};
		commandBuffer.addComponent(entities[i], pos);
	}

	// Commit the command buffer
	commandBuffer.commit();

	// Verify that the entities have the components and the data is correct
	for (size_t i = 0; i < entityCount; ++i)
	{
		ASSERT_TRUE(componentManager->hasComponent<PositionComponent>(entities[i]));
		PositionComponent& pos = componentManager->getComponent<PositionComponent>(entities[i]);
		ASSERT_EQ(pos.x, static_cast<float>(i));
		ASSERT_EQ(pos.y, static_cast<float>(i * 2));
		ASSERT_EQ(pos.z, static_cast<float>(i * 3));
	}
}

TEST_F(EcsTest, CommandBufferMultipleRemovals)
{
	auto entityManager = m_world->service()->entityManager();
	auto componentManager = m_world->service()->componentManager();

	const size_t entityCount = 5;
	spite::Entity entities[entityCount];

	// Create entities and add PositionComponent to them
	for (size_t i = 0; i < entityCount; ++i)
	{
		entities[i] = entityManager->createEntity();
		PositionComponent pos = {static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3)};
		componentManager->addComponent(entities[i], pos);
	}

	// Create a command buffer for removing PositionComponent
	auto commandBuffer = m_world->service()->getCommandBuffer<PositionComponent>();
	commandBuffer.reserveForRemoval(entityCount);

	// Remove components from the command buffer
	for (size_t i = 0; i < entityCount; ++i)
	{
		commandBuffer.removeComponent(entities[i]);
	}

	// Commit the command buffer
	commandBuffer.commit();

	// Verify that the entities no longer have the components
	for (size_t i = 0; i < entityCount; ++i)
	{
		ASSERT_FALSE(componentManager->hasComponent<PositionComponent>(entities[i]));
	}
}

TEST_F(EcsTest, QueryIgnoreDisabled)
{
	const sizet entityCount = 5;

	for (sizet i = 0; i < entityCount; ++i)
	{
		auto entity = m_world->service()->entityManager()->createEntity();
		PositionComponent position;
		position.isActive = false;
		m_world->service()->componentManager()->addComponent(entity, position);
	}

	auto queryInfo = m_world->service()->queryBuilder()->getQueryBuildInfo();
	auto* queryPtr = m_world->service()->queryBuilder()->buildQuery<PositionComponent>(queryInfo);
	auto& query = *queryPtr;

	sizet generalQuerySize = 0;
	for (auto positionComponent : query)
	{
		++generalQuerySize;
	}
	EXPECT_EQ(generalQuerySize, entityCount);

	sizet onlyActiveQuerySize = 0;
	for (auto component : query.excludeInactive())
	{
		++onlyActiveQuerySize;
	}
	EXPECT_EQ(onlyActiveQuerySize, 0);
}

//speed testing
TEST_F(EcsTest, BulkComponentStructuralChange)
{
	const sizet entityCount = 10000;
	spite::Entity entities[entityCount];
	for (sizet i = 0; i < entityCount; ++i)
	{
		entities[i] = m_world->service()->entityManager()->createEntity();
		PositionComponent position;
		m_world->service()->componentManager()->addComponent<PositionComponent>(entities[i],position);
	}
	for (sizet i = 0; i < entityCount; ++i)
	{
		m_world->service()->componentManager()->removeComponent<PositionComponent>(entities[i]);
	}
}
//speed testing
TEST_F(EcsTest, BulkCommandBufferStructuralChange)
{
	const sizet entityCount = 10000;
	spite::Entity entities[entityCount];

	m_world->service()->componentStorage()->registerComponent<PositionComponent>();
	{
		auto additionCbuf = m_world->service()->getCommandBuffer<PositionComponent>();
		additionCbuf.reserveForAddition(entityCount);
		for (sizet i = 0; i < entityCount; ++i)
		{
			entities[i] = m_world->service()->entityManager()->createEntity();
			PositionComponent position;
			additionCbuf.addComponent(entities[i], position);
		}
		additionCbuf.commit();
	}
	{
		auto removalCbuf = m_world->service()->getCommandBuffer<PositionComponent>();
		removalCbuf.reserveForRemoval(entityCount);
		for (sizet i = 0; i < entityCount; ++i)
		{
			removalCbuf.removeComponent(entities[i]);
		}
		removalCbuf.commit();
	}
}
