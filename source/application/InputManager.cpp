#include "InputManager.hpp"
#include "EventManager.hpp"
#include "Base/Logging.hpp"

namespace spite
{
	InputManager::InputManager()
	{
		m_keymap.insert(eastl::make_pair(SDLK_R, spite::Events::ROTATION_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_Q, spite::Events::NEXT_FIGURE_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_T, spite::Events::TRANSLATION_BUTTON_PRESS));
		//m_keymap.insert(eastl::make_pair(SDLK_S, spite::Events::SCALING_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_W, spite::Events::FWD_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_A, spite::Events::LFT_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_S, spite::Events::BCKWD_BUTTON_RESS));
		m_keymap.insert(eastl::make_pair(SDLK_D, spite::Events::RGHT_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_UP, spite::Events::LOOKUP_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_LEFT, spite::Events::LOOKLFT_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_RIGHT, spite::Events::LOOKRGHT_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_DOWN, spite::Events::LOOKDWN_BUTTON_PRESS));
	}

	Events InputManager::tryGetEvent(const u16 key)
	{
		if (!isKeyMapped(key))
		{
			SDEBUG_LOG("key %u is not present in the keymap!\n", key)
			return Events::NONE;
		}

		return m_keymap[key];
	}

	Events InputManager::getEvent(u16 key)
	{
		return m_keymap[key];
	}

	bool InputManager::isKeyMapped(const u16 key)
	{
		if (m_keymap.find(key) == m_keymap.end())
		{
			return false;
		}

		return true;
	}
}
