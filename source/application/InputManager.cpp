#include "InputManager.hpp"
#include "EventManager.hpp"
#include "Base/Logging.hpp"

namespace spite
{
	InputManager::InputManager()
	{
		m_keymap.insert(eastl::make_pair(SDLK_R, spite::InputEvents::ROTATION_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_Q, spite::InputEvents::NEXT_FIGURE_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_T, spite::InputEvents::TRANSLATION_BUTTON_PRESS));
		//m_keymap.insert(eastl::make_pair(SDLK_S, spite::Events::SCALING_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_W, spite::InputEvents::FWD_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_A, spite::InputEvents::LFT_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_S, spite::InputEvents::BCKWD_BUTTON_RESS));
		m_keymap.insert(eastl::make_pair(SDLK_D, spite::InputEvents::RGHT_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_UP, spite::InputEvents::LOOKUP_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_LEFT, spite::InputEvents::LOOKLFT_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_RIGHT, spite::InputEvents::LOOKRGHT_BUTTON_PRESS));
		m_keymap.insert(eastl::make_pair(SDLK_DOWN, spite::InputEvents::LOOKDWN_BUTTON_PRESS));
	}

	InputEvents InputManager::tryGetEvent(const u16 key)
	{
		if (!isKeyMapped(key))
		{
			SDEBUG_LOG("key %u is not present in the keymap!\n", key)
			return InputEvents::NONE;
		}

		return m_keymap[key];
	}

	InputEvents InputManager::getEvent(u16 key)
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
