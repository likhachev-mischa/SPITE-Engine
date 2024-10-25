#include "VulkanUsage.h"
#include "WindowManager.h"
#include "EventManager.h"
#include "AppConifg.h"

#include <GLFW/glfw3.h>


namespace application
{
	WindowManager::WindowManager(EventManager* eventManager) : m_eventManager(eventManager)
	{
		initWindow();
	}

	void WindowManager::initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);

		glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
		glfwSetKeyCallback(m_window, keyCallback);
	}

	void WindowManager::pollEvents()
	{
		glfwPollEvents();
	}

	void WindowManager::waitEvents()
	{
		glfwWaitEvents();
	}

	void WindowManager::getFramebufferSize(int& width, int& height)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
	}

	const char** WindowManager::getExtensions(uint32_t& extensionCount)
	{
		return glfwGetRequiredInstanceExtensions(&extensionCount);
	}

	bool WindowManager::shouldTerminate()
	{
		return glfwWindowShouldClose(m_window);
	}

	vk::SurfaceKHR WindowManager::createWindowSurface(vk::Instance& instance)
	{
		VkSurfaceKHR tempSurface;
		if (glfwCreateWindowSurface(instance, m_window, nullptr, &tempSurface) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create window surface!");
		}

		return tempSurface;
	}

	EventManager* WindowManager::getEventManager()
	{
		return m_eventManager;
	}

	WindowManager::~WindowManager()
	{
		glfwDestroyWindow(m_window);

		glfwTerminate();
	}

	void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto windowManager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
		EventManager* eventManager = windowManager->getEventManager();
		eventManager->triggerPollEvent(Events::FRAMEBUFFER_RESIZE);
		eventManager->triggerEvent(Events::FRAMEBUFFER_RESIZE);
	}

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action != GLFW_PRESS)
		{
			return;
		}
		auto windowManager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
		EventManager* eventManager = windowManager->getEventManager();

		switch (key)
		{
		case GLFW_KEY_R:
			eventManager->triggerEvent(Events::ROTATION_BUTTON_PRESS);
			eventManager->triggerPollEvent(Events::ROTATION_BUTTON_PRESS);
			break;
		case GLFW_KEY_T:
			eventManager->triggerEvent(Events::TRANSLATION_BUTTON_PRESS);
			eventManager->triggerPollEvent(Events::TRANSLATION_BUTTON_PRESS);
			break;
		case GLFW_KEY_S:
			eventManager->triggerEvent(Events::SCALING_BUTTON_PRESS);
			eventManager->triggerPollEvent(Events::SCALING_BUTTON_PRESS);
			break;
		case GLFW_KEY_N:
			eventManager->triggerEvent(Events::NEXT_FIGURE_BUTTON_PRESS);
			eventManager->triggerPollEvent(Events::NEXT_FIGURE_BUTTON_PRESS);
		default: break;
		}
	}
}
