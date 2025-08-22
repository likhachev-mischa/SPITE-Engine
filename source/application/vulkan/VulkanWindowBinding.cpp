#include "VulkanWindowBinding.hpp"
#include "base/Assert.hpp"
#include "base/VulkanUsage.hpp"
#include <SDL3/SDL_vulkan.h>

namespace spite
{
    VulkanWindowBinding::VulkanWindowBinding(SDL_Window* window)
        : m_window(window)
    {
        SASSERT(m_window);
    }

    char const* const* VulkanWindowBinding::getRequiredInstanceExtensions(u32& extensionCount) const
    {
        return SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    }

    vk::SurfaceKHR VulkanWindowBinding::createSurface(vk::Instance instance) const
    {
        VkSurfaceKHR surface;
        bool result = SDL_Vulkan_CreateSurface(m_window, instance, nullptr, &surface);
        SASSERTM(result, "Error on window surface creation!");
        return surface;
    }
}
