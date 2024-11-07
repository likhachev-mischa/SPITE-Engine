#pragma once
#include "Platform.hpp"

struct GLFWwindow;

namespace application
{
	class EventManager;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	class WindowManager
	{
	public:
		WindowManager(EventManager* eventManager);

		void initWindow();

		void pollEvents();

		void waitEvents();

		void getFramebufferSize(int& width, int& height);

		const char** getExtensions(u32& extensionCount);

		bool shouldTerminate();

		vk::SurfaceKHR createWindowSurface(vk::Instance& instance);

		EventManager* getEventManager();

		~WindowManager();

	private:
		GLFWwindow* m_window;
		EventManager* m_eventManager;
	};
}
