#pragma once
#include "base/VmaUsage.hpp"
#include "base/VulkanUsage.hpp"

#include "engine/rendering/GraphicsDescs.hpp"
#include "engine/rendering/GraphicsTypes.hpp"

namespace spite::vulkan
{
	// --- Usage Mappings ---

	inline vk::Rect2D to_vulkan_rect_2d(const Rect2D& rect)
	{
		return vk::Rect2D{
			{rect.offset.x, rect.offset.y},
			{rect.extent.width, rect.extent.height}
		};
	}

	inline vk::BufferUsageFlags to_vulkan_buffer_usage(BufferUsage usage)
	{
		vk::BufferUsageFlags flags;
		if (has_flag(usage, BufferUsage::TRANSFER_SRC)) flags |= vk::BufferUsageFlagBits::eTransferSrc;
		if (has_flag(usage, BufferUsage::TRANSFER_DST)) flags |= vk::BufferUsageFlagBits::eTransferDst;
		if (has_flag(usage, BufferUsage::UNIFORM_BUFFER)) flags |= vk::BufferUsageFlagBits::eUniformBuffer;
		if (has_flag(usage, BufferUsage::STORAGE_BUFFER)) flags |= vk::BufferUsageFlagBits::eStorageBuffer;
		if (has_flag(usage, BufferUsage::INDEX_BUFFER)) flags |= vk::BufferUsageFlagBits::eIndexBuffer;
		if (has_flag(usage, BufferUsage::VERTEX_BUFFER)) flags |= vk::BufferUsageFlagBits::eVertexBuffer;
		if (has_flag(usage, BufferUsage::INDIRECT_BUFFER)) flags |= vk::BufferUsageFlagBits::eIndirectBuffer;
		if (has_flag(usage, BufferUsage::RESOURCE_DESCRIPTOR_BUFFER)) flags |= vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT;
		if (has_flag(usage, BufferUsage::SAMPLER_DESCRIPTOR_BUFFER)) flags |= vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT;
		return flags;
	}

	inline VmaMemoryUsage to_vma_memory_usage(MemoryUsage usage)
	{
		switch (usage)
		{
		case MemoryUsage::GPU_ONLY: return VMA_MEMORY_USAGE_GPU_ONLY;
		case MemoryUsage::CPU_ONLY: return VMA_MEMORY_USAGE_CPU_ONLY;
		case MemoryUsage::CPU_TO_GPU: return VMA_MEMORY_USAGE_CPU_TO_GPU;
		case MemoryUsage::GPU_TO_CPU: return VMA_MEMORY_USAGE_GPU_TO_CPU;
		default: return VMA_MEMORY_USAGE_UNKNOWN;
		}
	}

	inline vk::ImageUsageFlags to_vulkan_texture_usage(TextureUsage usage)
	{
		vk::ImageUsageFlags flags;
		if (has_flag(usage, TextureUsage::TRANSFER_SRC)) flags |= vk::ImageUsageFlagBits::eTransferSrc;
		if (has_flag(usage, TextureUsage::TRANSFER_DST)) flags |= vk::ImageUsageFlagBits::eTransferDst;
		if (has_flag(usage, TextureUsage::SAMPLED)) flags |= vk::ImageUsageFlagBits::eSampled;
		if (has_flag(usage, TextureUsage::STORAGE)) flags |= vk::ImageUsageFlagBits::eStorage;
		if (has_flag(usage, TextureUsage::COLOR_ATTACHMENT)) flags |= vk::ImageUsageFlagBits::eColorAttachment;
		if (has_flag(usage, TextureUsage::DEPTH_STENCIL_ATTACHMENT)) flags |=
			vk::ImageUsageFlagBits::eDepthStencilAttachment;
		if (has_flag(usage, TextureUsage::INPUT_ATTACHMENT)) flags |= vk::ImageUsageFlagBits::eInputAttachment;
		return flags;
	}

	inline vk::DescriptorType to_vulkan_descriptor_type(DescriptorType type)
	{
		switch (type)
		{
		case DescriptorType::UNIFORM_BUFFER: return vk::DescriptorType::eUniformBuffer;
		case DescriptorType::STORAGE_BUFFER: return vk::DescriptorType::eStorageBuffer;
		case DescriptorType::SAMPLED_TEXTURE: return vk::DescriptorType::eCombinedImageSampler;
		case DescriptorType::STORAGE_TEXTURE: return vk::DescriptorType::eStorageImage;
		case DescriptorType::SAMPLER: return vk::DescriptorType::eSampler;
		default: SASSERT(false)
			return vk::DescriptorType::eUniformBuffer;
		}
	}

	inline vk::ShaderStageFlags to_vulkan_shader_stage(ShaderStage stages)
	{
		vk::ShaderStageFlags flags;
		if (has_flag(stages, ShaderStage::VERTEX)) flags |= vk::ShaderStageFlagBits::eVertex;
		if (has_flag(stages, ShaderStage::FRAGMENT)) flags |= vk::ShaderStageFlagBits::eFragment;
		if (has_flag(stages, ShaderStage::COMPUTE)) flags |= vk::ShaderStageFlagBits::eCompute;
		if (has_flag(stages, ShaderStage::GEOMETRY)) flags |= vk::ShaderStageFlagBits::eGeometry;
		if (has_flag(stages, ShaderStage::TESSELLATION_CONTROL)) flags |= vk::ShaderStageFlagBits::eTessellationControl;
		if (has_flag(stages, ShaderStage::TESSELLATION_EVALUATION)) flags |=
			vk::ShaderStageFlagBits::eTessellationEvaluation;
		return flags;
	}

	inline vk::ShaderStageFlagBits to_vulkan_shader_stage_bit(ShaderStage stage)
	{
		SASSERTM(std::has_single_bit(static_cast<u32>(stage)), "Shader stage must have a single bit set");
		switch (stage)
		{
		case ShaderStage::VERTEX: return vk::ShaderStageFlagBits::eVertex;
		case ShaderStage::FRAGMENT: return vk::ShaderStageFlagBits::eFragment;
		case ShaderStage::COMPUTE: return vk::ShaderStageFlagBits::eCompute;
		case ShaderStage::GEOMETRY: return vk::ShaderStageFlagBits::eGeometry;
		case ShaderStage::TESSELLATION_CONTROL: return vk::ShaderStageFlagBits::eTessellationControl;
		case ShaderStage::TESSELLATION_EVALUATION: return vk::ShaderStageFlagBits::eTessellationEvaluation;
		default: SASSERT(false)
			return vk::ShaderStageFlagBits::eVertex;
		}
	}

	inline vk::Format to_vulkan_format(Format format)
	{
		switch (format)
		{
		case Format::UNDEFINED: return vk::Format::eUndefined;
		case Format::R8_UNORM: return vk::Format::eR8Unorm;
		case Format::R8G8_UNORM: return vk::Format::eR8G8Unorm;
		case Format::R8G8B8A8_UNORM: return vk::Format::eR8G8B8A8Unorm;
		case Format::R8G8B8A8_SRGB: return vk::Format::eR8G8B8A8Srgb;
		case Format::B8G8R8A8_UNORM: return vk::Format::eB8G8R8A8Unorm;
		case Format::B8G8R8A8_SRGB: return vk::Format::eB8G8R8A8Srgb;
		case Format::R16_SFLOAT: return vk::Format::eR16Sfloat;
		case Format::R16G16_SFLOAT: return vk::Format::eR16G16Sfloat;
		case Format::R16G16B16A16_SFLOAT: return vk::Format::eR16G16B16A16Sfloat;
		case Format::R32_SFLOAT: return vk::Format::eR32Sfloat;
		case Format::R32G32_SFLOAT: return vk::Format::eR32G32Sfloat;
		case Format::R32G32B32_SFLOAT: return vk::Format::eR32G32B32Sfloat;
		case Format::R32G32B32A32_SFLOAT: return vk::Format::eR32G32B32A32Sfloat;
		case Format::D32_SFLOAT: return vk::Format::eD32Sfloat;
		case Format::D24_UNORM_S8_UINT: return vk::Format::eD24UnormS8Uint;
		case Format::D32_SFLOAT_S8_UINT: return vk::Format::eD32SfloatS8Uint;
		default: SASSERT(false)
			return vk::Format::eUndefined;
		}
	}

	inline vk::PipelineStageFlags to_vulkan_pipeline_stage(PipelineStage stages)
	{
		vk::PipelineStageFlags flags;
		if (has_flag(stages, PipelineStage::TOP_OF_PIPE)) flags |= vk::PipelineStageFlagBits::eTopOfPipe;
		if (has_flag(stages, PipelineStage::DRAW_INDIRECT)) flags |= vk::PipelineStageFlagBits::eDrawIndirect;
		if (has_flag(stages, PipelineStage::VERTEX_INPUT)) flags |= vk::PipelineStageFlagBits::eVertexInput;
		if (has_flag(stages, PipelineStage::VERTEX_SHADER)) flags |= vk::PipelineStageFlagBits::eVertexShader;
		if (has_flag(stages, PipelineStage::TESSELLATION_CONTROL_SHADER)) flags |=
			vk::PipelineStageFlagBits::eTessellationControlShader;
		if (has_flag(stages, PipelineStage::TESSELLATION_EVALUATION_SHADER)) flags |=
			vk::PipelineStageFlagBits::eTessellationEvaluationShader;
		if (has_flag(stages, PipelineStage::GEOMETRY_SHADER)) flags |= vk::PipelineStageFlagBits::eGeometryShader;
		if (has_flag(stages, PipelineStage::FRAGMENT_SHADER)) flags |= vk::PipelineStageFlagBits::eFragmentShader;
		if (has_flag(stages, PipelineStage::EARLY_FRAGMENT_TESTS)) flags |=
			vk::PipelineStageFlagBits::eEarlyFragmentTests;
		if (has_flag(stages, PipelineStage::LATE_FRAGMENT_TESTS)) flags |=
			vk::PipelineStageFlagBits::eLateFragmentTests;
		if (has_flag(stages, PipelineStage::COLOR_ATTACHMENT_OUTPUT)) flags |=
			vk::PipelineStageFlagBits::eColorAttachmentOutput;
		if (has_flag(stages, PipelineStage::COMPUTE_SHADER)) flags |= vk::PipelineStageFlagBits::eComputeShader;
		if (has_flag(stages, PipelineStage::TRANSFER)) flags |= vk::PipelineStageFlagBits::eTransfer;
		if (has_flag(stages, PipelineStage::BOTTOM_OF_PIPE)) flags |= vk::PipelineStageFlagBits::eBottomOfPipe;
		if (has_flag(stages, PipelineStage::HOST)) flags |= vk::PipelineStageFlagBits::eHost;
		if (has_flag(stages, PipelineStage::ALL_GRAPHICS)) flags |= vk::PipelineStageFlagBits::eAllGraphics;
		if (has_flag(stages, PipelineStage::ALL_COMMANDS)) flags |= vk::PipelineStageFlagBits::eAllCommands;
		return flags;
	}

	inline vk::PipelineStageFlags2 to_vulkan_pipeline_stage2(PipelineStage stages)
	{
		vk::PipelineStageFlags2 flags;
		if (has_flag(stages, PipelineStage::TOP_OF_PIPE)) flags |= vk::PipelineStageFlagBits2::eTopOfPipe;
		if (has_flag(stages, PipelineStage::DRAW_INDIRECT)) flags |= vk::PipelineStageFlagBits2::eDrawIndirect;
		if (has_flag(stages, PipelineStage::VERTEX_INPUT)) flags |= vk::PipelineStageFlagBits2::eVertexInput;
		if (has_flag(stages, PipelineStage::VERTEX_SHADER)) flags |= vk::PipelineStageFlagBits2::eVertexShader;
		if (has_flag(stages, PipelineStage::TESSELLATION_CONTROL_SHADER)) flags |=
			vk::PipelineStageFlagBits2::eTessellationControlShader;
		if (has_flag(stages, PipelineStage::TESSELLATION_EVALUATION_SHADER)) flags |=
			vk::PipelineStageFlagBits2::eTessellationEvaluationShader;
		if (has_flag(stages, PipelineStage::GEOMETRY_SHADER)) flags |= vk::PipelineStageFlagBits2::eGeometryShader;
		if (has_flag(stages, PipelineStage::FRAGMENT_SHADER)) flags |= vk::PipelineStageFlagBits2::eFragmentShader;
		if (has_flag(stages, PipelineStage::EARLY_FRAGMENT_TESTS)) flags |=
			vk::PipelineStageFlagBits2::eEarlyFragmentTests;
		if (has_flag(stages, PipelineStage::LATE_FRAGMENT_TESTS)) flags |=
			vk::PipelineStageFlagBits2::eLateFragmentTests;
		if (has_flag(stages, PipelineStage::COLOR_ATTACHMENT_OUTPUT)) flags |=
			vk::PipelineStageFlagBits2::eColorAttachmentOutput;
		if (has_flag(stages, PipelineStage::COMPUTE_SHADER)) flags |= vk::PipelineStageFlagBits2::eComputeShader;
		if (has_flag(stages, PipelineStage::TRANSFER)) flags |= vk::PipelineStageFlagBits2::eTransfer;
		if (has_flag(stages, PipelineStage::BOTTOM_OF_PIPE)) flags |= vk::PipelineStageFlagBits2::eBottomOfPipe;
		if (has_flag(stages, PipelineStage::HOST)) flags |= vk::PipelineStageFlagBits2::eHost;
		if (has_flag(stages, PipelineStage::ALL_GRAPHICS)) flags |= vk::PipelineStageFlagBits2::eAllGraphics;
		if (has_flag(stages, PipelineStage::ALL_COMMANDS)) flags |= vk::PipelineStageFlagBits2::eAllCommands;
		return flags;
	}

	inline vk::AccessFlags to_vulkan_access_flags(AccessFlags access)
	{
		vk::AccessFlags flags;
		if (has_flag(access, AccessFlags::INDIRECT_COMMAND_READ)) flags |= vk::AccessFlagBits::eIndirectCommandRead;
		if (has_flag(access, AccessFlags::INDEX_READ)) flags |= vk::AccessFlagBits::eIndexRead;
		if (has_flag(access, AccessFlags::VERTEX_ATTRIBUTE_READ)) flags |= vk::AccessFlagBits::eVertexAttributeRead;
		if (has_flag(access, AccessFlags::UNIFORM_READ)) flags |= vk::AccessFlagBits::eUniformRead;
		if (has_flag(access, AccessFlags::INPUT_ATTACHMENT_READ)) flags |= vk::AccessFlagBits::eInputAttachmentRead;
		if (has_flag(access, AccessFlags::SHADER_READ)) flags |= vk::AccessFlagBits::eShaderRead;
		if (has_flag(access, AccessFlags::SHADER_WRITE)) flags |= vk::AccessFlagBits::eShaderWrite;
		if (has_flag(access, AccessFlags::COLOR_ATTACHMENT_READ)) flags |= vk::AccessFlagBits::eColorAttachmentRead;
		if (has_flag(access, AccessFlags::COLOR_ATTACHMENT_WRITE)) flags |= vk::AccessFlagBits::eColorAttachmentWrite;
		if (has_flag(access, AccessFlags::DEPTH_STENCIL_ATTACHMENT_READ)) flags |=
			vk::AccessFlagBits::eDepthStencilAttachmentRead;
		if (has_flag(access, AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE)) flags |=
			vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		if (has_flag(access, AccessFlags::TRANSFER_READ)) flags |= vk::AccessFlagBits::eTransferRead;
		if (has_flag(access, AccessFlags::TRANSFER_WRITE)) flags |= vk::AccessFlagBits::eTransferWrite;
		if (has_flag(access, AccessFlags::HOST_READ)) flags |= vk::AccessFlagBits::eHostRead;
		if (has_flag(access, AccessFlags::HOST_WRITE)) flags |= vk::AccessFlagBits::eHostWrite;
		if (has_flag(access, AccessFlags::MEMORY_READ)) flags |= vk::AccessFlagBits::eMemoryRead;
		if (has_flag(access, AccessFlags::MEMORY_WRITE)) flags |= vk::AccessFlagBits::eMemoryWrite;
		return flags;
	}

	inline vk::AccessFlags2 to_vulkan_access_flags2(AccessFlags access)
	{
		vk::AccessFlags2 flags;
		if (has_flag(access, AccessFlags::INDIRECT_COMMAND_READ)) flags |= vk::AccessFlagBits2::eIndirectCommandRead;
		if (has_flag(access, AccessFlags::INDEX_READ)) flags |= vk::AccessFlagBits2::eIndexRead;
		if (has_flag(access, AccessFlags::VERTEX_ATTRIBUTE_READ)) flags |= vk::AccessFlagBits2::eVertexAttributeRead;
		if (has_flag(access, AccessFlags::UNIFORM_READ)) flags |= vk::AccessFlagBits2::eUniformRead;
		if (has_flag(access, AccessFlags::INPUT_ATTACHMENT_READ)) flags |= vk::AccessFlagBits2::eInputAttachmentRead;
		if (has_flag(access, AccessFlags::SHADER_READ)) flags |= vk::AccessFlagBits2::eShaderRead;
		if (has_flag(access, AccessFlags::SHADER_WRITE)) flags |= vk::AccessFlagBits2::eShaderWrite;
		if (has_flag(access, AccessFlags::COLOR_ATTACHMENT_READ)) flags |= vk::AccessFlagBits2::eColorAttachmentRead;
		if (has_flag(access, AccessFlags::COLOR_ATTACHMENT_WRITE)) flags |= vk::AccessFlagBits2::eColorAttachmentWrite;
		if (has_flag(access, AccessFlags::DEPTH_STENCIL_ATTACHMENT_READ)) flags |=
			vk::AccessFlagBits2::eDepthStencilAttachmentRead;
		if (has_flag(access, AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE)) flags |=
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
		if (has_flag(access, AccessFlags::TRANSFER_READ)) flags |= vk::AccessFlagBits2::eTransferRead;
		if (has_flag(access, AccessFlags::TRANSFER_WRITE)) flags |= vk::AccessFlagBits2::eTransferWrite;
		if (has_flag(access, AccessFlags::HOST_READ)) flags |= vk::AccessFlagBits2::eHostRead;
		if (has_flag(access, AccessFlags::HOST_WRITE)) flags |= vk::AccessFlagBits2::eHostWrite;
		if (has_flag(access, AccessFlags::MEMORY_READ)) flags |= vk::AccessFlagBits2::eMemoryRead;
		if (has_flag(access, AccessFlags::MEMORY_WRITE)) flags |= vk::AccessFlagBits2::eMemoryWrite;
		return flags;
	}

	inline vk::ImageLayout to_vulkan_image_layout(ImageLayout layout)
	{
		switch (layout)
		{
		case ImageLayout::UNDEFINED: return vk::ImageLayout::eUndefined;
		case ImageLayout::GENERAL: return vk::ImageLayout::eGeneral;
		case ImageLayout::COLOR_ATTACHMENT_OPTIMAL: return vk::ImageLayout::eColorAttachmentOptimal;
		case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
		case ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL: return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
		case ImageLayout::SHADER_READ_ONLY_OPTIMAL: return vk::ImageLayout::eShaderReadOnlyOptimal;
		case ImageLayout::TRANSFER_SRC_OPTIMAL: return vk::ImageLayout::eTransferSrcOptimal;
		case ImageLayout::TRANSFER_DST_OPTIMAL: return vk::ImageLayout::eTransferDstOptimal;
		case ImageLayout::PRESENT_SRC: return vk::ImageLayout::ePresentSrcKHR;
		default: SASSERT(false)
			return vk::ImageLayout::eUndefined;
		}
	}

	inline vk::PrimitiveTopology to_vulkan_primitive_topology(PrimitiveTopology topology)
	{
		switch (topology)
		{
		case PrimitiveTopology::POINT_LIST: return vk::PrimitiveTopology::ePointList;
		case PrimitiveTopology::LINE_LIST: return vk::PrimitiveTopology::eLineList;
		case PrimitiveTopology::LINE_STRIP: return vk::PrimitiveTopology::eLineStrip;
		case PrimitiveTopology::TRIANGLE_LIST: return vk::PrimitiveTopology::eTriangleList;
		case PrimitiveTopology::TRIANGLE_STRIP: return vk::PrimitiveTopology::eTriangleStrip;
		case PrimitiveTopology::TRIANGLE_FAN: return vk::PrimitiveTopology::eTriangleFan;
		default: SASSERT(false)
			return vk::PrimitiveTopology::eTriangleList;
		}
	}

	inline vk::PolygonMode to_vulkan_polygon_mode(PolygonMode mode)
	{
		switch (mode)
		{
		case PolygonMode::FILL: return vk::PolygonMode::eFill;
		case PolygonMode::LINE: return vk::PolygonMode::eLine;
		case PolygonMode::POINT: return vk::PolygonMode::ePoint;
		default: SASSERT(false)
			return vk::PolygonMode::eFill;
		}
	}

	inline vk::CullModeFlags to_vulkan_cull_mode(CullMode mode)
	{
		switch (mode)
		{
		case CullMode::NONE: return vk::CullModeFlagBits::eNone;
		case CullMode::FRONT: return vk::CullModeFlagBits::eFront;
		case CullMode::BACK: return vk::CullModeFlagBits::eBack;
		case CullMode::FRONT_AND_BACK: return vk::CullModeFlagBits::eFrontAndBack;
		default: SASSERT(false)
			return vk::CullModeFlagBits::eNone;
		}
	}

	inline vk::FrontFace to_vulkan_front_face(FrontFace face)
	{
		switch (face)
		{
		case FrontFace::COUNTER_CLOCKWISE: return vk::FrontFace::eCounterClockwise;
		case FrontFace::CLOCKWISE: return vk::FrontFace::eClockwise;
		default: SASSERT(false)
			return vk::FrontFace::eCounterClockwise;
		}
	}

	inline vk::CompareOp to_vulkan_compare_op(CompareOp op)
	{
		switch (op)
		{
		case CompareOp::NEVER: return vk::CompareOp::eNever;
		case CompareOp::LESS: return vk::CompareOp::eLess;
		case CompareOp::EQUAL: return vk::CompareOp::eEqual;
		case CompareOp::LESS_OR_EQUAL: return vk::CompareOp::eLessOrEqual;
		case CompareOp::GREATER: return vk::CompareOp::eGreater;
		case CompareOp::NOT_EQUAL: return vk::CompareOp::eNotEqual;
		case CompareOp::GREATER_OR_EQUAL: return vk::CompareOp::eGreaterOrEqual;
		case CompareOp::ALWAYS: return vk::CompareOp::eAlways;
		default: SASSERT(false)
			return vk::CompareOp::eAlways;
		}
	}

	inline vk::BlendFactor to_vulkan_blend_factor(BlendFactor factor)
	{
		switch (factor)
		{
		case BlendFactor::ZERO: return vk::BlendFactor::eZero;
		case BlendFactor::ONE: return vk::BlendFactor::eOne;
		case BlendFactor::SRC_COLOR: return vk::BlendFactor::eSrcColor;
		case BlendFactor::ONE_MINUS_SRC_COLOR: return vk::BlendFactor::eOneMinusSrcColor;
		case BlendFactor::DST_COLOR: return vk::BlendFactor::eDstColor;
		case BlendFactor::ONE_MINUS_DST_COLOR: return vk::BlendFactor::eOneMinusDstColor;
		case BlendFactor::SRC_ALPHA: return vk::BlendFactor::eSrcAlpha;
		case BlendFactor::ONE_MINUS_SRC_ALPHA: return vk::BlendFactor::eOneMinusSrcAlpha;
		case BlendFactor::DST_ALPHA: return vk::BlendFactor::eDstAlpha;
		case BlendFactor::ONE_MINUS_DST_ALPHA: return vk::BlendFactor::eOneMinusDstAlpha;
		default: SASSERT(false)
			return vk::BlendFactor::eZero;
		}
	}

	inline vk::BlendOp to_vulkan_blend_op(BlendOp op)
	{
		switch (op)
		{
		case BlendOp::ADD: return vk::BlendOp::eAdd;
		case BlendOp::SUBTRACT: return vk::BlendOp::eSubtract;
		case BlendOp::REVERSE_SUBTRACT: return vk::BlendOp::eReverseSubtract;
		case BlendOp::MIN: return vk::BlendOp::eMin;
		case BlendOp::MAX: return vk::BlendOp::eMax;
		default: SASSERT(false)
			return vk::BlendOp::eAdd;
		}
	}

	inline vk::Filter to_vulkan_filter(Filter filter)
	{
		switch (filter)
		{
		case Filter::NEAREST: return vk::Filter::eNearest;
		case Filter::LINEAR: return vk::Filter::eLinear;
		default: SASSERT(false)
			return vk::Filter::eLinear;
		}
	}

	inline vk::SamplerAddressMode to_vulkan_address_mode(AddressMode mode)
	{
		switch (mode)
		{
		case AddressMode::REPEAT: return vk::SamplerAddressMode::eRepeat;
		case AddressMode::MIRRORED_REPEAT: return vk::SamplerAddressMode::eMirroredRepeat;
		case AddressMode::CLAMP_TO_EDGE: return vk::SamplerAddressMode::eClampToEdge;
		case AddressMode::CLAMP_TO_BORDER: return vk::SamplerAddressMode::eClampToBorder;
		default: SASSERT(false)
			return vk::SamplerAddressMode::eRepeat;
		}
	}

	inline vk::BorderColor to_vulkan_border_color(BorderColor color)
	{
		switch (color)
		{
		case BorderColor::FLOAT_TRANSPARENT_BLACK: return vk::BorderColor::eFloatTransparentBlack;
		case BorderColor::INT_TRANSPARENT_BLACK: return vk::BorderColor::eIntTransparentBlack;
		case BorderColor::FLOAT_OPAQUE_BLACK: return vk::BorderColor::eFloatOpaqueBlack;
		case BorderColor::INT_OPAQUE_BLACK: return vk::BorderColor::eIntOpaqueBlack;
		case BorderColor::FLOAT_OPAQUE_WHITE: return vk::BorderColor::eFloatOpaqueWhite;
		case BorderColor::INT_OPAQUE_WHITE: return vk::BorderColor::eIntOpaqueWhite;
		default: SASSERT(false)
			return vk::BorderColor::eIntOpaqueBlack;
		}
	}

	inline u32 get_format_size(Format format)
	{
		switch (format)
		{
			case Format::R32_SFLOAT: return 4;
			case Format::R32G32_SFLOAT: return 8;
			case Format::R32G32B32_SFLOAT: return 12;
			case Format::R32G32B32A32_SFLOAT: return 16;
			default: SASSERT(false) return 0;
		}
	}

	inline vk::ImageAspectFlags to_vulkan_aspect_mask(Format format)
	{
		switch (format)
		{
			case Format::D32_SFLOAT:
				return vk::ImageAspectFlagBits::eDepth;
			case Format::D24_UNORM_S8_UINT:
			case Format::D32_SFLOAT_S8_UINT:
				return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			default:
				return vk::ImageAspectFlagBits::eColor;
		}
	}

	inline Format from_vulkan_format(vk::Format format)
	{
		switch (format)
		{
			case vk::Format::eUndefined: return Format::UNDEFINED;
			case vk::Format::eR8Unorm: return Format::R8_UNORM;
			case vk::Format::eR8G8Unorm: return Format::R8G8_UNORM;
			case vk::Format::eR8G8B8A8Unorm: return Format::R8G8B8A8_UNORM;
			case vk::Format::eR8G8B8A8Srgb: return Format::R8G8B8A8_SRGB;
			case vk::Format::eB8G8R8A8Unorm: return Format::B8G8R8A8_UNORM;
			case vk::Format::eB8G8R8A8Srgb: return Format::B8G8R8A8_SRGB;
			case vk::Format::eR16Sfloat: return Format::R16_SFLOAT;
			case vk::Format::eR16G16Sfloat: return Format::R16G16_SFLOAT;
			case vk::Format::eR16G16B16A16Sfloat: return Format::R16G16B16A16_SFLOAT;
			case vk::Format::eR32Sfloat: return Format::R32_SFLOAT;
			case vk::Format::eR32G32Sfloat: return Format::R32G32_SFLOAT;
			case vk::Format::eR32G32B32Sfloat: return Format::R32G32B32_SFLOAT;
			case vk::Format::eR32G32B32A32Sfloat: return Format::R32G32B32A32_SFLOAT;
			case vk::Format::eD32Sfloat: return Format::D32_SFLOAT;
			case vk::Format::eD24UnormS8Uint: return Format::D24_UNORM_S8_UINT;
			case vk::Format::eD32SfloatS8Uint: return Format::D32_SFLOAT_S8_UINT;
			default: SASSERT(false) return Format::UNDEFINED;
		}
	}
}
