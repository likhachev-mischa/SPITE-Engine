#pragma once
#include <cstdint>
#include <vector>
#include <tuple>
#include <queue>

struct EventContext;

namespace application
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
		void triggerPollEvent(const Events& eventParam);

		bool isPollEventTriggered(const Events& eventParam);

		void discardPollEvents();

		void subscribeToEvent(const Events& eventParam, void (*func)(EventContext&));

		void triggerEvent(const Events& eventParam);

		void processEvents(EventContext& context);

		~EventManager();

	private:
		uint64_t m_pollEventsMask = 0;
		std::vector<std::tuple<Events, void(*)(EventContext&)>> m_subscirbers;
		std::queue<void(*)(EventContext&)> m_executionQueue;
	};
}
