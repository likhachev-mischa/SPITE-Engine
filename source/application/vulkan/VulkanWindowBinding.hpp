#pragma once

#include "application/IWindowApiBinding.hpp"

#include "base/Platform.hpp"

namespace vk
{
	class Instance;
	class SurfaceKHR;
}

struct SDL_Window;

namespace spite
{
	class VulkanWindowBinding : public IWindowApiBinding
	{
	public:
		explicit VulkanWindowBinding(SDL_Window* window);

		char const* const* getRequiredInstanceExtensions(u32& extensionCount) const;
		vk::SurfaceKHR createSurface(vk::Instance instance) const;

	private:
		SDL_Window* m_window;
	};
}
