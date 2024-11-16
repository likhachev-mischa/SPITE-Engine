#pragma once
#include <cstdint>
#include <vector>
#include <tuple>
#include <queue>
#include <functional>
#include "Base/Platform.hpp"


namespace spite
{
	enum Events
	{
		FRAMEBUFFER_RESIZE = (1u << 0),
		ROTATION_BUTTON_PRESS = (1u << 1),
		SCALING_BUTTON_PRESS = (1u << 2),
		TRANSLATION_BUTTON_PRESS = (1u << 3),
		NEXT_FIGURE_BUTTON_PRESS = (1u << 4)
	};

	class EventManager
	{
	public:
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
