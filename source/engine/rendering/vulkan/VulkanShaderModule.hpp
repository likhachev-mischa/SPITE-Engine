#pragma once
#include "base/VulkanUsage.hpp"
#include "engine/rendering/IShaderModule.hpp"

namespace spite
{
    class VulkanShaderModule : public IShaderModule
    {
    public:
        VulkanShaderModule() = default;
        VulkanShaderModule(vk::Device device, const std::vector<char>& code);
        ~VulkanShaderModule() override;

        VulkanShaderModule(const VulkanShaderModule&) = delete;
        VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;
        VulkanShaderModule(VulkanShaderModule&& other) noexcept;
        VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

        vk::ShaderModule get() const { return m_module; }

    private:
        vk::Device m_device = nullptr;
        vk::ShaderModule m_module = nullptr;
    };
}