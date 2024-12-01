#pragma once
#include <optional>

#include "VulkanUsage.hpp"
#include "Base/Platform.hpp"

namespace spite
{
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
