#pragma once
#include <memory>

#include <SDL3/SDL_events.h>

#include "application/input/InputActionMap.hpp"

namespace spite
{
	class InputStateMap;
	class InputActionMap;

	class InputManager
	{
		std::shared_ptr<InputStateMap> m_inputStateMap;
		std::shared_ptr<InputActionMap> m_inputActionMap;

	public:
		InputManager(std::shared_ptr<InputActionMap> inputActionMap);

		//constexpr static sizet MAPPED_KEYS_COUNT = 11;

		//InputManager();

		////return NONE if key is unmapped//invalid
		//InputEvents tryGetEvent(u16 key);

		//InputEvents getEvent(u16 key);

		//bool isKeyMapped(u16 key);


		std::shared_ptr<InputStateMap> inputStateMap();

		std::shared_ptr<InputActionMap> inputActionMap();

		void triggerKeyInteraction(const SDL_KeyboardEvent& event);

		void reset();

	private:
		//	eastl::fixed_map<u16, InputEvents, MAPPED_KEYS_COUNT, false> m_keymap;
	};
}
