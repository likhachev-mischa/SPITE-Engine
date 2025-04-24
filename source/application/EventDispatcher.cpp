#include "EventDispatcher.hpp"

#include <SDL3/SDL_events.h>

#include "application/WindowManager.hpp"
#include "application/input/InputManager.hpp"

namespace spite
{
	//void EventManager::recordEvent(const InputEvents& eventId)
	//{
	//	m_recordedEvents.insert(eventId);
	//}

	//void EventManager::clearRecordedEvents()
	//{
	//	m_recordedEvents.reset_lose_memory();
	//}

	//eastl::fixed_set<InputEvents, INPUT_EVENT_COUNT, false>& EventManager::getRecordedEvents()
	//{
	//	return m_recordedEvents;
	//}

	//void EventManager::triggerPollEvent(const InputEvents& eventId)
	//{
	//	m_pollEventsMask |= eventId;
	//}

	//bool EventManager::isPollEventTriggered(const InputEvents& eventId)
	//{
	//	return (m_pollEventsMask & eventId) == eventId;
	//}

	//void EventManager::discardPollEvents()
	//{
	//	m_pollEventsMask = 0;
	//}

	//void EventManager::subscribeToEvent(const InputEvents& eventId, const std::function<void()>& callback)
	//{
	//	std::tuple tuple(eventId, callback);
	//	m_subscirbers.push_back(std::move(tuple));
	//}

	//void EventManager::triggerEvent(const InputEvents& eventId)
	//{
	//	for (const std::tuple<InputEvents, std::function<void()>>& eventTuple : m_subscirbers)
	//	{
	//		if (std::get<0>(eventTuple) == eventId)
	//		{
	//			m_executionQueue.push(std::get<1>(eventTuple));
	//		}
	//	}
	//}

	//void EventManager::processEvents()
	//{
	//	while (!m_executionQueue.empty())
	//	{
	//		m_executionQueue.front()();
	//		m_executionQueue.pop();
	//	}
	//}

	//EventManager::~EventManager()
	//{
	//	m_subscirbers.clear();

	//	while (!m_executionQueue.empty())
	//	{
	//		m_executionQueue.pop();
	//	}
	//}
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
			switch (event.type)
			{
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
				{
					m_inputManager->triggerKeyInteraction(event.key);
					break;
				}
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED :
				{
					m_windowManager->terminate();
					break;
				}

			default: break;
			}
		}
	}
}
