#include "GlobalTestEnvironment.hpp"

#include <base/memory/ScratchAllocator.hpp>

#include "base/memory/HeapAllocator.hpp"

namespace spite
{
    void GlobalEnvironment::SetUp()
    {
        initGlobalAllocator();
        FrameScratchAllocator::init();
    }

    void GlobalEnvironment::TearDown()
    {
        FrameScratchAllocator::shutdown();
        shutdownGlobalAllocator();
    }
}
