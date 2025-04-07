#pragma once
#include <cstdint>
#include <functional>
#include <queue>
#include <tuple>
#include <vector>

#include "InputEvents.hpp"
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

		void triggerPollEvent(const InputEvents& eventId);

		bool isPollEventTriggered(const InputEvents& eventId);

		void discardPollEvents();

		void subscribeToEvent(const InputEvents& eventId, const std::function<void()>& callback);

		void triggerEvent(const InputEvents& eventId);

		void processEvents();

		~EventManager();

	private:
		u64 m_pollEventsMask = 0;
		std::vector<std::tuple<InputEvents, std::function<void()>>> m_subscirbers;
		std::queue<std::function<void()>> m_executionQueue;
	};
}
