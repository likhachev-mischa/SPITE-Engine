
#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/systems/SystemBase.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"
using namespace spite::test;

// Test Systems
class MovementSystem : public spite::SystemBase {
public:
    using SystemBase::SystemBase;
    void onUpdate(float deltaTime) override {
        auto query = m_entityManager->getQueryBuilder().with<Position, Velocity>().build();
        for (auto [pos, vel] : query.view<Position, Velocity>()) {
            pos.x += vel.dx * deltaTime;
            pos.y += vel.dy * deltaTime;
            pos.z += vel.dz * deltaTime;
        }
    }
};

class EcsSystemTest : public testing::Test {
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsSystemTestAllocator", 32 * spite::MB)
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


	EcsSystemTest()
	{
	}

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsSystemTest, SystemUpdate) {
    auto entity = entityManager.createEntity();
    entityManager.addComponent<Position>(entity, Position());
    entityManager.addComponent<Velocity>(entity, 1.0f, 2.0f, 3.0f);

    MovementSystem movementSystem(&entityManager,&versionManager);
    movementSystem.onUpdate(1.0f);

    const auto& pos = entityManager.getComponent<Position>(entity);
    ASSERT_FLOAT_EQ(pos.x, 1.0f);
    ASSERT_FLOAT_EQ(pos.y, 2.0f);
    ASSERT_FLOAT_EQ(pos.z, 3.0f);
}

TEST_F(EcsSystemTest, SystemWithNoMatchingEntities) {
    auto entity = entityManager.createEntity();
	entityManager.addComponent<Position>(entity,Position());

    MovementSystem movementSystem(&entityManager,&versionManager);
    ASSERT_NO_THROW(movementSystem.onUpdate(1.0f));

    const auto& pos = entityManager.getComponent<Position>(entity);
    ASSERT_FLOAT_EQ(pos.x, 0.0f);
}