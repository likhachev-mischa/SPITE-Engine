#include "Time.hpp"
#include <SDL3/SDL.h>

namespace spite
{
	float Time::m_deltaTime = 0.0f;

	Time::Time()
	{
		m_performanceFrequency = SDL_GetPerformanceFrequency();
		m_lastCounter = SDL_GetPerformanceCounter();
		m_deltaTime = 0.0f;
	}

	float Time::deltaTime()
	{
		return m_deltaTime;
	}

	void Time::updateDeltaTime()
	{
		m_currentCounter = SDL_GetPerformanceCounter();
		m_deltaTime = static_cast<float>(m_currentCounter - m_lastCounter) / static_cast<float>(
			m_performanceFrequency);

		m_lastCounter = m_currentCounter;
	}
}
