#include "EventManager.h"

namespace application
{
	void EventManager::triggerPollEvent(const Events& eventParam)
	{
		m_pollEventsMask |= eventParam;
	}

	bool EventManager::isPollEventTriggered(const Events& eventParam)
	{
		return (m_pollEventsMask & eventParam) == eventParam;
	}

	void EventManager::discardPollEvents()
	{
		m_pollEventsMask = 0;
	}

	void EventManager::subscribeToEvent(const Events& eventParam, void(* func)(EventContext&))
	{
		std::tuple<Events, void(*)(EventContext&)> tuple(eventParam, func);
		m_subscirbers.push_back(tuple);
	}

	void EventManager::triggerEvent(const Events& eventParam)
	{
		for (const std::tuple<Events, void(*)(EventContext&)>& eventTuple : m_subscirbers)
		{
			if (std::get<0>(eventTuple) == eventParam)
			{
				m_executionQueue.push(std::get<1>(eventTuple));
			}
		}
	}

	void EventManager::processEvents(EventContext& context)
	{
		while (!m_executionQueue.empty())
		{
			m_executionQueue.front()(context);
			m_executionQueue.pop();
		}
	}

	EventManager::~EventManager()
	{
		m_subscirbers.clear();

		while (!m_executionQueue.empty())
		{
			m_executionQueue.pop();
		}
	}
}
