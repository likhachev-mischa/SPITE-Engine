#pragma once

#include "base/Platform.hpp"

namespace spite
{
	class Time
	{
		u64 m_performanceFrequency;
		u64 m_lastCounter;
		u64 m_currentCounter;
		static float m_deltaTime;
	public:
		Time();

		static float deltaTime();

		void updateDeltaTime();
	};
}
