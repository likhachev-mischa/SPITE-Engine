#pragma once
#include "Base/Platform.hpp"
#include "Base/VulkanUsage.hpp"


namespace spite
{
	inline constexpr u32 VK_API_VERSION = VK_API_VERSION_1_0;

	inline constexpr char APPLICATION_NAME[] = "Game";
	inline constexpr char ENGINE_NAME[] = "SPITE";

	inline constexpr u32 WIDTH = 800;
	inline constexpr u32 HEIGHT = 600;

	inline constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
}
