#pragma once
#include "Base/Logging.hpp"
#include "base/memory/ScratchAllocator.hpp"

namespace spite
{
    struct AllocatorStats
    {
        struct ScratchStats
        {
            sizet bytesUsed;
            sizet bytesTotal;
            sizet highWaterMark;
            float usagePercentage;
        };
        
        static ScratchStats getFrameScratchStats()
        {
            auto& scratch = FrameScratchAllocator::get();
            ScratchStats stats;
            stats.bytesUsed = scratch.bytes_used();
            stats.bytesTotal = scratch.total_size();
            stats.highWaterMark = scratch.high_water_mark();
            stats.usagePercentage = static_cast<float>(stats.bytesUsed) / stats.bytesTotal * 100.0f;
            return stats;
        }
        
        static void printFrameScratchStats()
        {
            auto stats = getFrameScratchStats();
            SDEBUG_LOG("Frame Scratch: %zu/%zu bytes (%.1f%%), HWM: %zu bytes\n",
                   stats.bytesUsed, stats.bytesTotal, stats.usagePercentage, stats.highWaterMark);
        }
    };
    
   }
