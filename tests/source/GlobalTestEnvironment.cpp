#include "GlobalTestEnvironment.hpp"

#include <base/memory/ScratchAllocator.hpp>

#include "base/memory/HeapAllocator.hpp"
#include "ecs/core/ComponentMetadataRegistry.hpp"

namespace spite
{
    void GlobalEnvironment::SetUp()
    {
        initGlobalAllocator();
        FrameScratchAllocator::init();
        ComponentMetadataRegistry::init(getGlobalAllocator());
    }

    void GlobalEnvironment::TearDown()
    {
        ComponentMetadataRegistry::destroy();
        FrameScratchAllocator::shutdown();
        shutdownGlobalAllocator();
    }
}
