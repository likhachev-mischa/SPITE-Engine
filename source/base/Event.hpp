#pragma once
#include <functional>
#include <vector>

template <typename... Args>
class Event
{
public:
	using HandlerType = std::function<void(Args...)>;

	void addHandler(HandlerType handler)
	{
		m_handlers.push_back(std::move(handler));
	}

	void invoke(Args... args)
	{
		for (auto& handler : m_handlers)
		{
			handler(args...);
		}
	}

private:
	std::vector<HandlerType> m_handlers;
};
