
#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"

// A component with a constructor, destructor, and move constructor to track its lifecycle.
struct LifecycleComponent : spite::IComponent
{
    static thread_local int constructor_calls;
    static thread_local int destructor_calls;
	static thread_local int move_constructor_calls;

    int* data;

    LifecycleComponent() : data(new int(10)) {
        constructor_calls++;
    }

    ~LifecycleComponent() {
        destructor_calls++;
        delete data;
    }

    LifecycleComponent(const LifecycleComponent&) = delete;
    LifecycleComponent& operator=(const LifecycleComponent&) = delete;

    LifecycleComponent(LifecycleComponent&& other) noexcept : data(other.data) {
        move_constructor_calls++;
        other.data = nullptr;
    }

    LifecycleComponent& operator=(LifecycleComponent&& other) noexcept {
        if (this != &other) {
            delete data;
            data = other.data;
            other.data = nullptr;
        }
        return *this;
    }
};

thread_local int LifecycleComponent::constructor_calls = 0;
thread_local int LifecycleComponent::destructor_calls = 0;
thread_local int LifecycleComponent::move_constructor_calls = 0;

struct OtherComponent : spite::IComponent {};

class EcsLifecycleTest : public testing::Test
{
protected:
	struct Allocators
	{
		spite::HeapAllocator allocator;

		Allocators()
			: allocator("EcsLifecycleTestAllocator", 32 * spite::MB)
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
			componentInitializer.registerComponent<LifecycleComponent>();
			componentInitializer.registerComponent<OtherComponent>();
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


	EcsLifecycleTest()
	{
	}

	void SetUp() override {
        LifecycleComponent::constructor_calls = 0;
        LifecycleComponent::destructor_calls = 0;
        LifecycleComponent::move_constructor_calls = 0;
    }

	void TearDown() override
	{
		allocator.delete_object(container);
		delete allocContainer;
	}
};

TEST_F(EcsLifecycleTest, ComponentConstructorIsCalledOnAdd)
{
    auto entity = entityManager.createEntity();
    entityManager.addComponent<LifecycleComponent>(entity);
    ASSERT_EQ(LifecycleComponent::constructor_calls, 1);
    ASSERT_EQ(LifecycleComponent::destructor_calls, 0);
}

TEST_F(EcsLifecycleTest, ComponentDestructorIsCalledOnRemove)
{
    auto entity = entityManager.createEntity();
    entityManager.addComponent<LifecycleComponent>(entity);
    entityManager.removeComponent<LifecycleComponent>(entity);
    ASSERT_EQ(LifecycleComponent::constructor_calls, 1);
    ASSERT_EQ(LifecycleComponent::destructor_calls, 1);
}

TEST_F(EcsLifecycleTest, ComponentDestructorIsCalledOnDestroy)
{
    auto entity = entityManager.createEntity();
    entityManager.addComponent<LifecycleComponent>(entity);
    entityManager.destroyEntity(entity);
    ASSERT_EQ(LifecycleComponent::constructor_calls, 1);
    ASSERT_EQ(LifecycleComponent::destructor_calls, 1);
}

TEST_F(EcsLifecycleTest, ComponentIsMovedOnArchetypeChange)
{
    auto entity = entityManager.createEntity();
    entityManager.addComponent<LifecycleComponent>(entity);
    ASSERT_EQ(LifecycleComponent::constructor_calls, 1);

    // This will move the entity to a new archetype
    entityManager.addComponent<OtherComponent>(entity);

    // The original component is move-constructed into the new archetype, and then the old one is destructed.
    ASSERT_EQ(LifecycleComponent::move_constructor_calls, 1);
    ASSERT_EQ(LifecycleComponent::destructor_calls, 1);
}
