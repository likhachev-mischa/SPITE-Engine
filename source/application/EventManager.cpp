#include "EventManager.hpp"

namespace spite
{
	void EventManager::triggerPollEvent(const InputEvents& eventId)
	{
		m_pollEventsMask |= eventId;
	}

	bool EventManager::isPollEventTriggered(const InputEvents& eventId)
	{
		return (m_pollEventsMask & eventId) == eventId;
	}

	void EventManager::discardPollEvents()
	{
		m_pollEventsMask = 0;
	}

	void EventManager::subscribeToEvent(const InputEvents& eventId, const std::function<void()>& callback)
	{
		std::tuple tuple(eventId, callback);
		m_subscirbers.push_back(std::move(tuple));
	}

	void EventManager::triggerEvent(const InputEvents& eventId)
	{
		for (const std::tuple<InputEvents, std::function<void()>>& eventTuple : m_subscirbers)
		{
			if (std::get<0>(eventTuple) == eventId)
			{
				m_executionQueue.push(std::get<1>(eventTuple));
			}
		}
	}

	void EventManager::processEvents()
	{
		while (!m_executionQueue.empty())
		{
			m_executionQueue.front()();
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
