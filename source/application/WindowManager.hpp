#pragma once
#include <SDL3/SDL.h>

#include "Base/Platform.hpp"
#include "Base/VulkanUsage.hpp"

namespace spite
{
	//class InputManager;
	//class EventManager;

	class WindowManager
	{
	public:
		WindowManager(const WindowManager& other) = delete;
		WindowManager(WindowManager&& other) = delete;
		WindowManager& operator=(const WindowManager& other) = delete;
		WindowManager& operator=(WindowManager&& other) = delete;

	//	WindowManager(std::shared_ptr<EventManager> eventManager, std::shared_ptr<InputManager> inputManager);
		WindowManager();

		void processEvent(const SDL_Event& event);

		void waitWindowExpand() const;

		void getFramebufferSize(int& width, int& height) const;
		bool isMinimized() const;

		char const* const* getExtensions(u32& extensionCount) const;

		bool shouldTerminate() const;

		vk::SurfaceKHR createWindowSurface(const vk::Instance& instance,
		                                   vk::AllocationCallbacks* allocationCallbacks = nullptr);

		~WindowManager();

	private:
		SDL_Window* m_window{};
		//std::shared_ptr<EventManager> m_eventManager;
		//std::shared_ptr<InputManager> m_inputManager;

		bool m_shouldTerminate = false;

		void initWindow();
	};
}
