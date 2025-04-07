#pragma once
#include <optional>

#include "Base/Assert.hpp"
#include "Base/Platform.hpp"
#include "Base/VulkanUsage.hpp"

namespace spite
{
#define SASSERT_VULKAN(result) SASSERTM((result) == vk::Result::eSuccess, "Vulkan assertion failed %u",result)

	struct QueueFamilyIndices
	{
		std::optional<u32> graphicsFamily;
		std::optional<u32> presentFamily;
		std::optional<u32> transferFamily;

		bool isComplete() const
		{
			return graphicsFamily.has_value() && presentFamily.has_value() &&
				transferFamily.has_value();
		}
	};

	struct SwapchainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats{};
		std::vector<vk::PresentModeKHR> presentModes{};
	};

}
