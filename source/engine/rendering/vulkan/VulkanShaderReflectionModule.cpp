#include "VulkanShaderReflectionModule.hpp"
#include "base/Assert.hpp"
#include <spirv-reflect/spirv_reflect.h>

namespace spite
{
	VulkanShaderReflectionModule::VulkanShaderReflectionModule(const std::vector<char>& spirvBinary,
	                                                           ShaderStage shaderStage)
		: m_shaderStage(shaderStage)
	{
		SpvReflectResult result = spvReflectCreateShaderModule(spirvBinary.size(), spirvBinary.data(),
		                                                       &m_reflectionModule);
		SASSERTM(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to create SPIR-V reflection module.")
	}

	VulkanShaderReflectionModule::~VulkanShaderReflectionModule()
	{
		spvReflectDestroyShaderModule(&m_reflectionModule);
	}

	ShaderReflectionData VulkanShaderReflectionModule::reflect() const
	{
		ShaderReflectionData data;

		// --- Reflect Descriptor Sets ---
		u32 setCount = 0;
		spvReflectEnumerateDescriptorSets(&m_reflectionModule, &setCount, nullptr);
		std::vector<SpvReflectDescriptorSet*> spvSets(setCount);
		spvReflectEnumerateDescriptorSets(&m_reflectionModule, &setCount, spvSets.data());

		for (const auto* spvSet : spvSets)
		{
			ResourceSetLayoutDescription layoutDesc;
			for (u32 i = 0; i < spvSet->binding_count; ++i)
			{
				const auto* spvBinding = spvSet->bindings[i];
				ResourceBinding binding;
				binding.binding = spvBinding->binding;
				binding.descriptorCount = spvBinding->count;
				binding.type = to_engine_descriptor_type(spvBinding->descriptor_type);
				binding.shaderStages = m_shaderStage;
				if (binding.type == DescriptorType::UNIFORM_BUFFER || binding.type == DescriptorType::STORAGE_BUFFER)
				{
					binding.size = spvBinding->block.size;
					binding.name = spvBinding->block.name;
				}
				else
				{
					binding.name = spvBinding->name;
				}
				layoutDesc.bindings.push_back(binding);
			}
			data.resourceSetLayouts[spvSet->set] = layoutDesc;
		}

		// --- Reflect Vertex Attributes (only for vertex shaders) ---
		if (m_shaderStage == ShaderStage::VERTEX)
		{
			u32 inputCount = 0;
			spvReflectEnumerateInputVariables(&m_reflectionModule, &inputCount, nullptr);
			std::vector<SpvReflectInterfaceVariable*> spvInputs(inputCount);
			spvReflectEnumerateInputVariables(&m_reflectionModule, &inputCount, spvInputs.data());

			for (const auto* spvVar : spvInputs)
			{
				// Skip built-in variables
				if (spvVar->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) continue;

				VertexInputAttributeDesc attribute;
				attribute.location = spvVar->location;
				attribute.format = to_engine_format(spvVar->format);
				// Binding and offset are now left to the user to define in the PipelineDescription.
				attribute.binding = -1; // Set to an invalid value to indicate it's not set here.
				attribute.offset = 0;
				data.vertexInputAttributes.push_back(attribute);
			}
		}

		// --- Reflect Push Constants ---
		u32 blockCount = 0;
		spvReflectEnumeratePushConstantBlocks(&m_reflectionModule, &blockCount, nullptr);
		std::vector<SpvReflectBlockVariable*> spvBlocks(blockCount);
		spvReflectEnumeratePushConstantBlocks(&m_reflectionModule, &blockCount, spvBlocks.data());

		for (const auto* spvBlock : spvBlocks)
		{
			PushConstantRange range;
			range.shaderStages = m_shaderStage;
			range.offset = spvBlock->offset;
			range.size = spvBlock->size;
			data.pushConstantRanges.push_back(range);
		}

		return data;
	}

	DescriptorType VulkanShaderReflectionModule::to_engine_descriptor_type(u32 spvReflectType)
	{
		switch (static_cast<SpvReflectDescriptorType>(spvReflectType))
		{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return DescriptorType::SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return DescriptorType::SAMPLED_TEXTURE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return DescriptorType::SAMPLED_TEXTURE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return DescriptorType::STORAGE_TEXTURE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return DescriptorType::UNIFORM_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return DescriptorType::STORAGE_BUFFER;
		default: SASSERTM(false, "Unsupported descriptor type")
			return DescriptorType::UNIFORM_BUFFER;
		}
	}

	Format VulkanShaderReflectionModule::to_engine_format(u32 spvReflectFormat)
	{
		switch (static_cast<SpvReflectFormat>(spvReflectFormat))
		{
		case SPV_REFLECT_FORMAT_UNDEFINED: return Format::UNDEFINED;
		case SPV_REFLECT_FORMAT_R32_SFLOAT: return Format::R32_SFLOAT;
		case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return Format::R32G32_SFLOAT;
		case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return Format::R32G32B32_SFLOAT;
		// Note: Mapping 3-component to 4-component for alignment
		case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return Format::R32G32B32A32_SFLOAT;
		// Add more format mappings as needed
		default: SASSERTM(false, "Unsupported vertex format")
			return Format::UNDEFINED;
		}
	}
}
