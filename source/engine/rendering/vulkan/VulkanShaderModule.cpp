#include "VulkanShaderModule.hpp"

#include "base/Assert.hpp"
#include "base/Platform.hpp"

namespace spite
{
	VulkanShaderModule::VulkanShaderModule(vk::Device device, const std::vector<char>& code)
		: m_device(device)
	{
		vk::ShaderModuleCreateInfo createInfo{};
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const u32*>(code.data());

		auto module = m_device.createShaderModule(createInfo);
		SASSERT_VULKAN(module.result)
		m_module = module.value;
	}

	VulkanShaderModule::~VulkanShaderModule()
	{
		if (m_module)
		{
			m_device.destroyShaderModule(m_module);
		}
	}

	VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept
		: m_device(other.m_device),
		  m_module(other.m_module)
	{
		other.m_device = nullptr;
		other.m_module = nullptr;
	}

	VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept
	{
		if (this != &other)
		{
			if (m_module) { m_device.destroyShaderModule(m_module); }
			m_device = other.m_device;
			m_module = other.m_module;
			other.m_device = nullptr;
			other.m_module = nullptr;
		}
		return *this;
	}
}
