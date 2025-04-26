#include "InputManager.hpp"

#include "application/Time.hpp"
#include "InputActionMap.hpp"


namespace spite
{
	//InputManager::InputManager()
	//{
	//	m_keymap.insert(eastl::make_pair(SDLK_R, spite::InputEvents::ROTATION_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_Q, spite::InputEvents::NEXT_FIGURE_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_T, spite::InputEvents::TRANSLATION_BUTTON_PRESS));
	//	//m_keymap.insert(eastl::make_pair(SDLK_S, spite::Events::SCALING_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_W, spite::InputEvents::FWD_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_A, spite::InputEvents::LFT_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_S, spite::InputEvents::BCKWD_BUTTON_RESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_D, spite::InputEvents::RGHT_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_UP, spite::InputEvents::LOOKUP_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_LEFT, spite::InputEvents::LOOKLFT_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_RIGHT, spite::InputEvents::LOOKRGHT_BUTTON_PRESS));
	//	m_keymap.insert(eastl::make_pair(SDLK_DOWN, spite::InputEvents::LOOKDWN_BUTTON_PRESS));
	//}

	//InputEvents InputManager::tryGetEvent(const u16 key)
	//{
	//	if (!isKeyMapped(key))
	//	{
	//		SDEBUG_LOG("key %u is not present in the keymap!\n", key)
	//		return InputEvents::NONE;
	//	}

	//	return m_keymap[key];
	//}

	//InputEvents InputManager::getEvent(u16 key)
	//{
	//	return m_keymap[key];
	//}

	//bool InputManager::isKeyMapped(const u16 key)
	//{
	//	if (m_keymap.find(key) == m_keymap.end())
	//	{
	//		return false;
	//	}


	//	return true;
	//}

	InputManager::InputManager(std::shared_ptr<InputActionMap> inputActionMap): m_inputActionMap(
		std::move(inputActionMap))
	{
		m_inputStateMap = std::make_shared<InputStateMap>();
	}

	void InputManager::update(const float deltaTime) 
	{
		m_mousePosition.xRel = 0.0f;
		m_mousePosition.yRel = 0.0f;

		m_inputStateMap->updateActiveKeys(deltaTime);
		auto& activeKeys = m_inputStateMap->activeKeys();

		for (u32 activeKey : activeKeys)
		{
			m_inputActionMap->triggerKeyInteraction(activeKey, true, true, m_inputStateMap->holdingTime(activeKey));
		}
	}

	std::shared_ptr<InputStateMap> InputManager::inputStateMap()
	{
		return m_inputStateMap;
	}

	std::shared_ptr<InputActionMap> InputManager::inputActionMap()
	{
		return m_inputActionMap;
	}

	void InputManager::setMousePosition(const float x,
		const float y,
		const float xRel,
		const float yRel)
	{
		m_mousePosition.x = x;
		m_mousePosition.y = y;
		m_mousePosition.xRel = xRel;
		m_mousePosition.yRel = yRel;
	}

	const MousePosition& InputManager::mousePosition() const
	{
		return m_mousePosition;
	}

	void InputManager::triggerKeyInteraction(const u32 key,const bool isPressed)
	{
		if (isPressed)
		{
			m_inputStateMap->setButtonPress(key);
			m_inputActionMap->triggerKeyInteraction(key,
			                                        isPressed,
			                                        m_inputStateMap->isHeld(key),
			                                        m_inputStateMap->holdingTime(key));
		}
		else
		{
			m_inputStateMap->setButtonRelease(key);
			m_inputActionMap->triggerKeyInteraction(key, isPressed, false, 0.0f);
		}
	}

	void InputManager::reset()
	{
		m_inputStateMap->reset();
		m_inputActionMap->reset();
	}
}
