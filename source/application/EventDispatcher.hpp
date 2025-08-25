#pragma once
#include <memory>

namespace spite
{
	class WindowManager;
	class InputManager;

	class EventDispatcher
	{
		std::shared_ptr<InputManager> m_inputManager;
		std::shared_ptr<WindowManager> m_windowManager;

	public:
		EventDispatcher(const EventDispatcher& other) = delete;
		EventDispatcher(EventDispatcher&& other) = delete;
		EventDispatcher& operator=(const EventDispatcher& other) = delete;
		EventDispatcher& operator=(EventDispatcher&& other) = delete;
		~EventDispatcher() = default;

		EventDispatcher(std::shared_ptr<InputManager> inputManager,
		                std::shared_ptr<WindowManager> windowManager);

		void pollEvents();
	};
}
