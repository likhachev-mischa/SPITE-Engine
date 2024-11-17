#pragma once
#include <cstdint>
#include <functional>
#include <queue>
#include <tuple>
#include <vector>

#include "Events.hpp"
#include "Base/Platform.hpp"

namespace spite
{
	class EventManager
	{
	public:
		EventManager(const EventManager& other) = delete;
		EventManager(EventManager&& other) = delete;
		EventManager& operator=(const EventManager& other) = delete;
		EventManager& operator=(EventManager&& other) = delete;

		EventManager() = default;

		void triggerPollEvent(const Events& eventId);

		bool isPollEventTriggered(const Events& eventId);

		void discardPollEvents();

		void subscribeToEvent(const Events& eventId, const std::function<void()>& callback);

		void triggerEvent(const Events& eventId);

		void processEvents();

		~EventManager();

	private:
		u64 m_pollEventsMask = 0;
		std::vector<std::tuple<Events, std::function<void()>>> m_subscirbers;
		std::queue<std::function<void()>> m_executionQueue;
	};
}
