#pragma once
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT ;
#include <vulkan/vulkan.hpp>


namespace spite
{
#define SASSERT_VULKAN(result) SASSERTM((result) == vk::Result::eSuccess, "Vulkan assertion failed %u",result)
}
