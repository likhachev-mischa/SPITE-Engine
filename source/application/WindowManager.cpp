#include "WindowManager.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_vulkan.h>

#include "Application/AppConifg.hpp"

#include "Base/Assert.hpp"
#include "Base/Logging.hpp"

namespace spite
{
	//WindowManager::WindowManager(std::shared_ptr<EventManager> eventManager,
	//                             std::shared_ptr<InputManager> inputManager) :
	//	m_eventManager(std::move(eventManager)),
	//	m_inputManager(std::move(inputManager))
	//{
	//	initWindow();
	//}

	WindowManager::WindowManager()
	{
		initWindow();
	}

	void WindowManager::processEvent(const SDL_Event& event)
	{
		if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
		{
			m_shouldTerminate = true;
		}
	}

	void WindowManager::initWindow()
	{
		bool result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
		SASSERTM(result, "Error on SDL initializaion!");

		result = SDL_SetAppMetadata(APPLICATION_NAME, "0.1", "aboba");
		SASSERTM(result, "Error on SDL set metadata!");

		m_window = SDL_CreateWindow("SPITE", WIDTH, HEIGHT,SDL_WINDOW_VULKAN |
		                            SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS);
		SASSERTM(m_window != nullptr, "Error on SDL Vulkan window creation!")
	}

	void WindowManager::waitWindowExpand() const
	{
		SDL_Event event;
		while (SDL_WaitEvent(&event))
		{
			if (event.type == SDL_EVENT_WINDOW_RESTORED)
				return;
		}
	}

	void WindowManager::getFramebufferSize(int& width, int& height) const
	{
		SDL_GetWindowSize(m_window, &width, &height);
	}

	bool WindowManager::isMinimized() const
	{
		return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED);
	}

	char const* const* WindowManager::getExtensions(u32& extensionCount) const
	{
		return SDL_Vulkan_GetInstanceExtensions(&extensionCount);
	}

	bool WindowManager::shouldTerminate() const
	{
		return m_shouldTerminate;
	}

	vk::SurfaceKHR WindowManager::createWindowSurface(const vk::Instance& instance,
	                                                  vk::AllocationCallbacks* allocationCallbacks)
	{
		VkSurfaceKHR surface;
		bool result = SDL_Vulkan_CreateSurface(m_window, instance,
		                                       //   reinterpret_cast<VkAllocationCallbacks*>(allocationCallbacks),
		                                       nullptr,
		                                       &surface);
		SASSERTM(result, "Error on window surface creation!")

		return surface;
	}

	WindowManager::~WindowManager()
	{
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}
}
