#pragma once
#include "RenderResourceHandles.hpp"
#include "GraphicsDescs.hpp"
#include "base/CollectionAliases.hpp"
#include <EASTL/span.h>

#include "base/HashedString.hpp"
#include "base/StringInterner.hpp"

namespace spite
{
	class IPipeline;

	struct PipelineShaderStageDesc
	{
		ShaderModuleHandle module;
		ShaderStage stage;
		HashedString entryPoint = toHashedString("main");

		bool operator==(const PipelineShaderStageDesc& other) const = default;
	};

	// Describes all necessary information to create a graphics or compute pipeline.
	// This struct is used as the key in the PipelineCache.
	struct PipelineDescription
	{
		sbo_vector<ResourceSetLayoutHandle> resourceSetLayouts;
		sbo_vector<PushConstantRange> pushConstantRanges;
		RenderPassHandle renderPass;
		sbo_vector<PipelineShaderStageDesc> shaderStages;

		// Vertex Input
		sbo_vector<VertexInputBindingDesc> vertexBindings;
		sbo_vector<VertexInputAttributeDesc> vertexAttributes;

		// Key pipeline state
		PrimitiveTopology topology = PrimitiveTopology::TRIANGLE_LIST;
		PolygonMode polygonMode = PolygonMode::FILL;
		CullMode cullMode = CullMode::BACK;
		FrontFace frontFace = FrontFace::COUNTER_CLOCKWISE;
		bool depthTestEnable = true;
		bool depthWriteEnable = true;
		CompareOp depthCompareOp = CompareOp::LESS;

		sbo_vector<BlendStateDesc> blendStates; // One per color attachment

		// --- Dynamic Rendering State ---
		sbo_vector<Format> colorAttachmentFormats;
		Format depthAttachmentFormat = Format::UNDEFINED;

		bool operator==(const PipelineDescription& other) const;

		struct Hash
		{
			sizet operator()(const PipelineDescription& desc) const;
		};
	};

	struct ComputePipelineDescription
	{
		sbo_vector<ResourceSetLayoutHandle> resourceSetLayouts;
		PipelineShaderStageDesc shaderStage;

		bool operator==(const ComputePipelineDescription& other) const;

		struct Hash
		{
			sizet operator()(const ComputePipelineDescription& desc) const;
		};
	};

	// Describes a single shader stage for pipeline creation from source files.
	struct ShaderStageDescription
	{
		HashedString path;
		ShaderStage stage;
		HashedString entryPoint = toHashedString("main");
	};

	class IPipelineCache
	{
	public:
		virtual ~IPipelineCache() = default;

		virtual PipelineHandle getOrCreatePipeline(const PipelineDescription& description) = 0;
		virtual PipelineHandle getOrCreatePipeline(const ComputePipelineDescription& description) = 0;

		virtual PipelineHandle getOrCreatePipelineFromShaders(
			eastl::span<const ShaderStageDescription> shaderStages,
			const PipelineDescription& baseDescription
		) = 0;

		virtual void destroyPipeline(PipelineHandle handle) = 0;

		virtual IPipeline& getPipeline(PipelineHandle handle) = 0;
		virtual const IPipeline& getPipeline(PipelineHandle handle) const = 0;
		virtual const PipelineDescription& getPipelineDescription(PipelineHandle handle) const = 0;
		virtual PipelineLayoutHandle getPipelineLayoutHandle(PipelineHandle handle) const = 0;
	};
}
