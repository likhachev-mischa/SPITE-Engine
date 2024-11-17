#pragma once
#include "Base/Platform.hpp"
#include "Engine/VulkanUsage.hpp"
#include <SDL3/SDL.h>

namespace spite
{
	class InputManager;
	class EventManager;

	class WindowManager
	{
	public:
		WindowManager(const WindowManager& other) = delete;
		WindowManager(WindowManager&& other) = delete;
		WindowManager& operator=(const WindowManager& other) = delete;
		WindowManager& operator=(WindowManager&& other) = delete;

		explicit WindowManager(EventManager* eventManager, InputManager* inputManager);

		void initWindow();

		void pollEvents();

		void waitWindowExpand();

		void getFramebufferSize(int& width, int& height);
		bool isMinimized();

		char const* const* getExtensions(u32& extensionCount);

		bool shouldTerminate();

		vk::SurfaceKHR createWindowSurface(const vk::Instance& instance);

		~WindowManager();

	private:
		SDL_Window* m_window{};
		EventManager* m_eventManager;
		InputManager* m_inputManager;
		bool m_shouldTerminate = false;
	};
}
