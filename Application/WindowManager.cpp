#include "WindowManager.hpp"
#include "EventManager.hpp"
#include "AppConifg.hpp"
#include "InputManager.hpp"

#include "Base/Assert.hpp"
#include "Base/Logging.hpp"

#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_events.h>

namespace spite
{
	WindowManager::WindowManager(EventManager* eventManager, InputManager* inputManager) : m_eventManager(eventManager),
		m_inputManager(inputManager)
	{
		initWindow();
	}

	void WindowManager::initWindow()
	{
		bool result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
		SASSERTM(result, "Error on SDL initializaion!");

		result = SDL_SetAppMetadata(APPLICATION_NAME, "0.1", "aboba");
		SASSERTM(result, "Error on SDL set metadata!");

		m_window = SDL_CreateWindow("SPITE", WIDTH, HEIGHT,SDL_WINDOW_VULKAN |
		                            SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS );
		SASSERTM(m_window != nullptr, "Error on SDL Vulkan window creation!")
	}

	void WindowManager::pollEvents()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EVENT_KEY_UP:
				{
					Events buttonEvent = m_inputManager->tryGetEvent(event.key.key);
					if (buttonEvent != Events::NONE)
					{
						m_eventManager->triggerEvent(buttonEvent);
						m_eventManager->triggerPollEvent(buttonEvent);
					}
					break;
				}
			case SDL_EVENT_QUIT:
				{
					m_shouldTerminate = true;
					break;
				}

			default: break;
			}
		}
	}

	void WindowManager::waitWindowExpand()
	{
		SDL_Event event;
		while (SDL_WaitEvent(&event))
		{
			if (event.type == SDL_EVENT_WINDOW_RESTORED)
				return;
		}
	}

	void WindowManager::getFramebufferSize(int& width, int& height)
	{
		SDL_GetWindowSize(m_window, &width, &height);
	}

	bool WindowManager::isMinimized()
	{
		return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED);
	}

	char const* const* WindowManager::getExtensions(uint32_t& extensionCount)
	{
		return SDL_Vulkan_GetInstanceExtensions(&extensionCount);
	}

	bool WindowManager::shouldTerminate()
	{
		return m_shouldTerminate;
	}

	vk::SurfaceKHR WindowManager::createWindowSurface(const vk::Instance& instance)
	{
		VkSurfaceKHR tempSurface;
		bool result = SDL_Vulkan_CreateSurface(m_window, instance, nullptr, &tempSurface);
		SASSERTM(result, "Error on window surface creation!")

		return tempSurface;
	}

	//TODO: destroy surface
	WindowManager::~WindowManager()
	{
		SDL_DestroyWindow(m_window);
	//	SDL_Vulkan_DestroySurface();
		SDL_Quit();
	}
}
