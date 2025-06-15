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

		//	void recordEvent(const InputEvents& eventId);

		//	void clearRecordedEvents();

		//	eastl::fixed_set<InputEvents, INPUT_EVENT_COUNT, false>& getRecordedEvents();

		//	void triggerPollEvent(const InputEvents& eventId);

		//	bool isPollEventTriggered(const InputEvents& eventId);

		//	void discardPollEvents();

		//	void subscribeToEvent(const InputEvents& eventId, const std::function<void()>& callback);

		//	void triggerEvent(const InputEvents& eventId);

		//	void processEvents();

		//	~EventManager();

		//private:
		//	u64 m_pollEventsMask = 0;
		//	std::vector<std::tuple<InputEvents, std::function<void()>>> m_subscirbers;
		//	std::queue<std::function<void()>> m_executionQueue;

		//	//used for recording events every frame for external processing
		//	eastl::fixed_set<InputEvents, INPUT_EVENT_COUNT, false> m_recordedEvents;
	};
}
