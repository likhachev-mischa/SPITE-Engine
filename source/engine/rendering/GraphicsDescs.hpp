#pragma once
#include "GraphicsTypes.hpp"
#include "RenderResourceHandles.hpp"
#include "base/CollectionAliases.hpp"
#include <variant>

namespace spite
{
    struct Offset2D
    {
        i32 x = 0;
        i32 y = 0;
    };

    struct Extent2D
    {
        u32 width = 0;
        u32 height = 0;
    };

    struct Rect2D
    {
        Offset2D offset;
        Extent2D extent;
    };

    struct ClearColorValue
    {
        float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
    };

    struct ClearDepthStencilValue
    {
        float depth = 1.0f;
        u32 stencil = 0;
    };

    using ClearValue = std::variant<ClearColorValue, ClearDepthStencilValue>;

    struct BufferDesc
    {
        sizet size = 0;
        BufferUsage usage = BufferUsage::NONE;
        MemoryUsage memoryUsage = MemoryUsage::UNKNOWN;
    };

    struct TextureDesc
    {
        u32 width = 1;
        u32 height = 1;
        u32 depth = 1;
        u32 mipLevels = 1;
        u32 arrayLayers = 1;
        Format format = Format::UNDEFINED;
        TextureUsage usage = TextureUsage::NONE;
        MemoryUsage memoryUsage = MemoryUsage::GPU_ONLY;
    };

    struct SamplerDesc
    {
        Filter magFilter = Filter::LINEAR;
        Filter minFilter = Filter::LINEAR;
        AddressMode addressModeU = AddressMode::REPEAT;
        AddressMode addressModeV = AddressMode::REPEAT;
        AddressMode addressModeW = AddressMode::REPEAT;
        bool anisotropyEnable = true;
        float maxAnisotropy = 16.0f;
        BorderColor borderColor = BorderColor::INT_OPAQUE_BLACK;
        CompareOp compareOp = CompareOp::ALWAYS;
        bool compareEnable = false;
    };

    enum class VertexInputRate : u8
    {
        VERTEX,
        INSTANCE
    };

    struct VertexInputBindingDesc
    {
        u32 binding = 0;
        u32 stride = 0;
        VertexInputRate inputRate = VertexInputRate::VERTEX;

        bool operator==(const VertexInputBindingDesc& other) const = default;
    };

    struct VertexInputAttributeDesc
    {
        u32 location = 0;
        u32 binding = 0;
        Format format = Format::UNDEFINED;
        u32 offset = 0;

        bool operator==(const VertexInputAttributeDesc& other) const = default;
    };

    struct ImageViewDesc
    {
        TextureHandle texture;
        Format format = Format::UNDEFINED; // Can be different from texture format for compatibility
        u32 baseMipLevel = 0;
        u32 levelCount = 1;
        u32 baseArrayLayer = 0;
        u32 layerCount = 1;
    };

    struct AttachmentDesc
    {
        Format format = Format::UNDEFINED;
        AttachmentLoadOp loadOp = AttachmentLoadOp::DONT_CARE;
        AttachmentStoreOp storeOp = AttachmentStoreOp::DONT_CARE;
        ImageLayout initialLayout = ImageLayout::UNDEFINED;
        ImageLayout finalLayout = ImageLayout::UNDEFINED;

        bool operator==(const AttachmentDesc& other) const = default;
    };

    struct AttachmentReference
    {
        u32 attachmentIndex = -1; // Index into the RenderPass's attachment array
        ImageLayout layout = ImageLayout::UNDEFINED;

        bool operator==(const AttachmentReference& other) const = default;
    };

    struct SubpassDesc
    {
        sbo_vector<AttachmentReference> colorAttachments;
        sbo_vector<AttachmentReference> resolveAttachments;
        AttachmentReference depthStencilAttachment;

        bool operator==(const SubpassDesc& other) const = default;
    };

    struct SubpassDependencyDesc
    {
        u32 srcSubpass = -1; // VK_SUBPASS_EXTERNAL
        u32 dstSubpass = -1;
        PipelineStage srcStageMask = PipelineStage::TOP_OF_PIPE;
        PipelineStage dstStageMask = PipelineStage::BOTTOM_OF_PIPE;
        AccessFlags srcAccessMask = AccessFlags::NONE;
        AccessFlags dstAccessMask = AccessFlags::NONE;

        bool operator==(const SubpassDependencyDesc& other) const = default;
    };

    struct BlendStateDesc
    {
        bool blendEnable = false;
        BlendFactor srcColorBlendFactor = BlendFactor::ONE;
        BlendFactor dstColorBlendFactor = BlendFactor::ZERO;
        BlendOp colorBlendOp = BlendOp::ADD;
        BlendFactor srcAlphaBlendFactor = BlendFactor::ONE;
        BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
        BlendOp alphaBlendOp = BlendOp::ADD;

        bool operator==(const BlendStateDesc& other) const = default;
    };

    struct PushConstantRange
    {
        ShaderStage shaderStages = ShaderStage::NONE;
        u32 offset = 0;
        u32 size = 0;

        bool operator==(const PushConstantRange& other) const = default;
    };

} 
