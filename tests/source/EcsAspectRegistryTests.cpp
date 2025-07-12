
#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "base/memory/HeapAllocator.hpp"

class EcsAspectRegistryTest : public testing::Test
{
protected:
    spite::HeapAllocator allocator;
    spite::AspectRegistry* registry;

    EcsAspectRegistryTest() : allocator("EcsAspectRegistryTestAllocator", 16 * spite::MB) {
        registry = allocator.new_object<spite::AspectRegistry>(allocator);
    }

    ~EcsAspectRegistryTest() override {
        allocator.delete_object(registry);
        allocator.shutdown();
    }
};

TEST_F(EcsAspectRegistryTest, AddAndGetAspect)
{
    spite::Aspect aspect({1, 2, 3});
    const spite::Aspect* registeredAspect = registry->addOrGetAspect(aspect);
    ASSERT_NE(registeredAspect, nullptr);
    ASSERT_EQ(*registeredAspect, aspect);
    ASSERT_TRUE(registry->hasAspect(aspect));
}

TEST_F(EcsAspectRegistryTest, GetNonExistentAspect)
{
    spite::Aspect aspect({4, 5, 6});
    ASSERT_FALSE(registry->hasAspect(aspect));
    ASSERT_EQ(registry->getAspect(aspect), nullptr);
}

TEST_F(EcsAspectRegistryTest, AddAndRemoveAspect)
{
    spite::Aspect aspect({1, 2, 3});
    registry->addOrGetAspect(aspect);
    ASSERT_TRUE(registry->hasAspect(aspect));
    ASSERT_TRUE(registry->removeAspect(aspect));
    ASSERT_FALSE(registry->hasAspect(aspect));
}

TEST_F(EcsAspectRegistryTest, RemoveNonExistentAspect)
{
    spite::Aspect aspect({1, 2, 3});
    ASSERT_FALSE(registry->removeAspect(aspect));
}

TEST_F(EcsAspectRegistryTest, GetDescendants)
{
    spite::Aspect parent({1, 2});
    spite::Aspect child({1, 2, 3});
    registry->addOrGetAspect(parent);
    registry->addOrGetAspect(child);

    auto descendants = registry->getDescendantAspects(parent);
    ASSERT_EQ(descendants.size(), 1);
    ASSERT_EQ(*descendants[0], child);
}

TEST_F(EcsAspectRegistryTest, GetAncestors)
{
    spite::Aspect parent({1, 2});
    spite::Aspect child({1, 2, 3});
    registry->addOrGetAspect(parent);
    registry->addOrGetAspect(child);

    auto ancestors = registry->getAncestorsAspects(child);

    //parent + empty aspect
    ASSERT_EQ(ancestors.size(), 2);
    ASSERT_TRUE(*ancestors[0]== parent);
}
