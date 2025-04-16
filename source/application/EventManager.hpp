#pragma once
#include <cstdint>
#include <functional>
#include <queue>
#include <tuple>
#include <vector>

#include <EASTL/fixed_set.h>
#include <EASTL/hash_map.h>

#include "InputEvents.hpp"

#include "base/Memory.hpp"
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

		void recordEvent(const InputEvents& eventId);

		void clearRecordedEvents();

		eastl::fixed_set<InputEvents, INPUT_EVENT_COUNT, false>& getRecordedEvents();

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

		//used for recording events every frame for external processing
		eastl::fixed_set<InputEvents, INPUT_EVENT_COUNT, false> m_recordedEvents;
	};
}
