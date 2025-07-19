#include <gtest/gtest.h>
#include "ecs/core/SingletonComponentRegistry.hpp"
#include "ecs/core/EntityManager.hpp"
#include "ecs/config/TestComponents.hpp"
#include "base/memory/HeapAllocator.hpp"
#include "ecs/core/EntityWorld.hpp"

using namespace spite;
using namespace spite::test;

class EcsSingletonComponentTest : public testing::Test
{
protected:
	SingletonComponentRegistry registry;

	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsSingletonTestAllocator", 32 * spite::MB)
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

	EntityManager& entityManager = container->entityManager;
};

TEST_F(EcsSingletonComponentTest, GetCreatesInstance)
{
	TestSingletonA& singleton = registry.get<TestSingletonA>();
	ASSERT_EQ(singleton.value, 10);
}

TEST_F(EcsSingletonComponentTest, GetReturnsSameInstance)
{
	TestSingletonA& s1 = registry.get<TestSingletonA>();
	TestSingletonA& s2 = registry.get<TestSingletonA>();
	ASSERT_EQ(&s1, &s2);
}

TEST_F(EcsSingletonComponentTest, ModifyAndGet)
{
	TestSingletonA& s1 = registry.get<TestSingletonA>();
	s1.value = 42;

	TestSingletonA& s2 = registry.get<TestSingletonA>();
	ASSERT_EQ(s2.value, 42);
}

TEST_F(EcsSingletonComponentTest, RegisterAndGet)
{
	auto new_singleton = std::make_unique<TestSingletonA>();
	new_singleton->value = 100;

	registry.registerSingleton(std::move(new_singleton));

	TestSingletonA& s1 = registry.get<TestSingletonA>();
	ASSERT_EQ(s1.value, 100);
}

TEST_F(EcsSingletonComponentTest, MultipleSingletons)
{
	TestSingletonA& sA = registry.get<TestSingletonA>();
	TestSingletonB& sB = registry.get<TestSingletonB>();

	sA.value = 111;
	sB.value = 222.0f;

	ASSERT_EQ(registry.get<TestSingletonA>().value, 111);
	ASSERT_FLOAT_EQ(registry.get<TestSingletonB>().value, 222.0f);
}

TEST_F(EcsSingletonComponentTest, EntityManagerIntegration)
{
    TestSingletonA& s1 = entityManager.getSingletonComponent<TestSingletonA>();
    s1.value = 55;

    TestSingletonA& s2 = entityManager.getSingletonComponent<TestSingletonA>();
    ASSERT_EQ(s2.value, 55);
    ASSERT_EQ(&s1, &s2);
}
