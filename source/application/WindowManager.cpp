#include "WindowManager.hpp"
#include "vulkan/VulkanWindowBinding.hpp"
#include "Application/AppConifg.hpp"
#include "Base/Assert.hpp"
#include "Base/Logging.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

namespace spite
{
	WindowManager::WindowManager(GraphicsApi api)
	{
		u64 windowFlags = 0;
		switch (api)
		{
		case GraphicsApi::Vulkan:
			windowFlags = SDL_WINDOW_VULKAN;
			break;
		default:
			SASSERTM(false, "Unsupported Graphics API");
			break;
		}

		initWindow(windowFlags);

		switch (api)
		{
		case GraphicsApi::Vulkan:
			m_binding = std::make_unique<VulkanWindowBinding>(m_window);
			break;
		default:
			break;
		}
	}

	void WindowManager::terminate()
	{
		m_shouldTerminate = true;
	}

	void WindowManager::initWindow(u64 apiFlag)
	{
		bool result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
		SASSERTM(result, "Error on SDL initialization!");

		m_window = SDL_CreateWindow(APPLICATION_NAME,
		                            WIDTH,
		                            HEIGHT,
		                            apiFlag | SDL_WINDOW_INPUT_FOCUS |
		                            SDL_WINDOW_MOUSE_FOCUS);
		SASSERTM(m_window != nullptr, "Error on SDL window creation!")
	}

	void WindowManager::waitWindowExpand() const
	{
		SDL_Event event;
		while (SDL_WaitEvent(&event))
		{
			if (event.type == SDL_EVENT_WINDOW_RESTORED) return;
		}
	}

	void WindowManager::getFramebufferSize(int& width, int& height) const
	{
		SDL_GetWindowSizeInPixels(m_window, &width, &height);
	}

	bool WindowManager::isMinimized() const
	{
		return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED);
	}

	bool WindowManager::shouldTerminate() const
	{
		return m_shouldTerminate;
	}

	IWindowApiBinding* WindowManager::getBinding() const
	{
		return m_binding.get();
	}

	SDL_Window* WindowManager::getWindow() const
	{
		return m_window;
	}

	WindowManager::~WindowManager()
	{
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}
}
