
#include <gtest/gtest.h>
#include "ecs/core/EntityWorld.hpp"
#include "ecs/query/QueryBuilder.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "base/memory/HeapAllocator.hpp"

class EcsVersionManagerTest : public testing::Test
{
protected:
    spite::HeapAllocator allocator;
    spite::AspectRegistry* aspectRegistry;
    spite::VersionManager* versionManager;

    EcsVersionManagerTest() : allocator("EcsVersionManagerTestAllocator", 16 * spite::MB) {
        aspectRegistry = allocator.new_object<spite::AspectRegistry>(allocator);
        versionManager = allocator.new_object<spite::VersionManager>(allocator, aspectRegistry);
    }

    ~EcsVersionManagerTest() override {
        allocator.delete_object(versionManager);
        allocator.delete_object(aspectRegistry);
        allocator.shutdown();
    }
};

TEST_F(EcsVersionManagerTest, InitialVersionIsZero)
{
    spite::Aspect aspect({1, 2, 3});
    aspectRegistry->addOrGetAspect(aspect);
    ASSERT_EQ(versionManager->getVersion(aspect), 0);
}

TEST_F(EcsVersionManagerTest, DirtyMakesVersionNonZero)
{
    spite::Aspect aspect({1, 2, 3});
    aspectRegistry->addOrGetAspect(aspect);
    versionManager->makeDirty(aspect);
    ASSERT_NE(versionManager->getVersion(aspect), 0);
}

TEST_F(EcsVersionManagerTest, DirtyIncrementsVersion)
{
    spite::Aspect aspect({1, 2, 3});
    aspectRegistry->addOrGetAspect(aspect);
    versionManager->makeDirty(aspect);
    u64 version1 = versionManager->getVersion(aspect);
    versionManager->makeDirty(aspect);
    u64 version2 = versionManager->getVersion(aspect);
    ASSERT_GT(version2, version1);
}

TEST_F(EcsVersionManagerTest, DirtyAffectsAncestors)
{
    spite::Aspect parent({1, 2});
    spite::Aspect child({1, 2, 3});
    aspectRegistry->addOrGetAspect(parent);
    aspectRegistry->addOrGetAspect(child);

    u64 parentVersion1 = versionManager->getVersion(parent);
    versionManager->makeDirty(child);
    u64 parentVersion2 = versionManager->getVersion(parent);
    ASSERT_GT(parentVersion2, parentVersion1);
}
