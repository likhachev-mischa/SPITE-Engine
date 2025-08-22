#pragma once
#include "base/Platform.hpp"
#include "GraphicsTypes.hpp"
#include "RenderResourceHandles.hpp"

namespace spite
{
    constexpr sizet WHOLE_SIZE = ~0ULL;

    struct MemoryBarrier2
    {
        PipelineStage srcStageMask = PipelineStage::NONE;
        AccessFlags srcAccessMask = AccessFlags::NONE;
        PipelineStage dstStageMask = PipelineStage::NONE;
        AccessFlags dstAccessMask = AccessFlags::NONE;
    };

    struct BufferBarrier2
    {
        BufferHandle buffer;
        PipelineStage srcStageMask = PipelineStage::NONE;
        AccessFlags srcAccessMask = AccessFlags::NONE;
        PipelineStage dstStageMask = PipelineStage::NONE;
        AccessFlags dstAccessMask = AccessFlags::NONE;
        sizet offset = 0;
        sizet size = WHOLE_SIZE; 
    };

    struct TextureBarrier2
    {
        TextureHandle texture;
        PipelineStage srcStageMask = PipelineStage::NONE;
        AccessFlags srcAccessMask = AccessFlags::NONE;
        PipelineStage dstStageMask = PipelineStage::NONE;
        AccessFlags dstAccessMask = AccessFlags::NONE;
        ImageLayout oldLayout = ImageLayout::UNDEFINED;
        ImageLayout newLayout = ImageLayout::UNDEFINED;
        u32 baseMipLevel = 0;
        u32 levelCount = 1;
        u32 baseArrayLayer = 0;
        u32 layerCount = 1;
    };

}
