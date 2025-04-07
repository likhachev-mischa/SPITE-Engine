#pragma once
#include <EASTL/fixed_map.h>
#include <SDL3/SDL_keycode.h>

#include "InputEvents.hpp"
#include "Base/Platform.hpp"

namespace spite
{
	//TODO: map inputs from config file
	class InputManager
	{
	public:
		constexpr static sizet MAPPED_KEYS_COUNT = 11;

		InputManager();

		//return NONE if key is unmapped//invalid
		InputEvents tryGetEvent(u16 key);

		InputEvents getEvent(u16 key);

		bool isKeyMapped(u16 key);

	private:
		eastl::fixed_map<u16, InputEvents, MAPPED_KEYS_COUNT, false> m_keymap;
	};
}
