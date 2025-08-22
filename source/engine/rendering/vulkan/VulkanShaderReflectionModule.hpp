#pragma once
#include "engine/rendering/GraphicsDescs.hpp"
#include "engine/rendering/IResourceSetLayoutCache.hpp"
#include "base/CollectionAliases.hpp"
#include <vector>

#include <spirv-reflect/spirv_reflect.h>

namespace spite
{
    // Holds all the data extracted from a single shader module.
    struct ShaderReflectionData
    {
        // Using a map to keep layouts organized by their set number.
        heap_unordered_map<u32, ResourceSetLayoutDescription> resourceSetLayouts;
        sbo_vector<VertexInputAttributeDesc> vertexInputAttributes;
        sbo_vector<PushConstantRange> pushConstantRanges; 
    };

    // A wrapper around the SPIRV-Reflect library to extract pipeline layout
    // information from SPIR-V bytecode.
    class VulkanShaderReflectionModule
    {
    public:
        // The constructor takes the raw SPIR-V bytecode.
        VulkanShaderReflectionModule(const std::vector<char>& spirvBinary, ShaderStage shaderStage);
        ~VulkanShaderReflectionModule();

        VulkanShaderReflectionModule(const VulkanShaderReflectionModule&) = delete;
        VulkanShaderReflectionModule& operator=(const VulkanShaderReflectionModule&) = delete;
        VulkanShaderReflectionModule(VulkanShaderReflectionModule&&) = delete;
        VulkanShaderReflectionModule& operator=(VulkanShaderReflectionModule&&) = delete;

        // Performs the reflection and returns the extracted data in engine-native structs.
        ShaderReflectionData reflect() const;

    private:
        SpvReflectShaderModule m_reflectionModule;
        ShaderStage m_shaderStage;

        // Helper methods to translate from SPIRV-Reflect types to our engine types.
        static DescriptorType to_engine_descriptor_type(u32 spvReflectType);
        static Format to_engine_format(u32 spvReflectFormat);
    };
}
