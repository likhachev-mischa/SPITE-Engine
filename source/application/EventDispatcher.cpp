#include "EventDispatcher.hpp"

#include <SDL3/SDL_events.h>

#include "application/WindowManager.hpp"
#include "application/input/InputManager.hpp"


#include "engine/ui/UIInspectorManager.hpp"

namespace spite
{
	EventDispatcher::EventDispatcher(std::shared_ptr<InputManager> inputManager,
	                                 std::shared_ptr<WindowManager> windowManager):
		m_inputManager(std::move(inputManager)), m_windowManager(std::move(windowManager))
	{
	}

	void EventDispatcher::pollEvents()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			//TODO refactor
			UIInspectorManager::get()->processEvent(&event);
			switch (event.type)
			{
			case SDL_EVENT_MOUSE_MOTION:
				{
					auto motion = event.motion;
					m_inputManager->setMousePosition(motion.x, motion.y, motion.xrel, motion.yrel);
					break;
				}

			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
				{
					m_inputManager->triggerKeyInteraction(event.key.key, event.key.down);
					if (event.key.key == SDLK_F && event.key.down)
					{
						UIInspectorManager::get()->setActive(!UIInspectorManager::get()->isActive());
					}

					break;
				}
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				{
					m_windowManager->terminate();
					break;
				}

			default: break;
			}
		}
	}
}
