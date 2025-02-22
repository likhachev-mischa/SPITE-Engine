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
		//spite::getGlobalAllocator().shutdown();
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
	sizet entitesCount = 10;
	auto* entities = new spite::Entity[entitesCount];

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

	auto additionCbuf = m_world->service()->getCommandBuffer<test1::Component>();
	additionCbuf.reserveForAddition(entitesCount);
	m_world->service()->componentStorage()->registerComponent<test1::Component>();

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
