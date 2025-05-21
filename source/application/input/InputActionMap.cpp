#include "InputActionMap.hpp"

#include "base/File.hpp"

namespace spite
{
	ButtonState::ButtonState(): state(BUTTON_STATE::IDLE)
	{
	}

	ButtonState::ButtonState(const BUTTON_STATE state, const float holdingTime): state(state),
		holdingTime(holdingTime)
	{
	}

	bool InputStateMap::isInactive(const u32 key)
	{
		if (m_stateMap.find(key) == m_stateMap.end())
		{
			registerKey(key);
			return true;
		}
		return m_stateMap[key].state == BUTTON_STATE::IDLE;
	}

	bool InputStateMap::isPressed(const u32 key)
	{
		if (isInactive(key))
		{
			return false;
		}

		return m_stateMap[key].state == BUTTON_STATE::PRESSED;
	}

	bool InputStateMap::isReleased(const u32 key)
	{
		if (isInactive(key))
		{
			return false;
		}

		return m_stateMap[key].state == BUTTON_STATE::RELEASED;
	}

	bool InputStateMap::isHeld(const u32 key)
	{
		if (isInactive(key))
		{
			return false;
		}

		return m_stateMap[key].state == BUTTON_STATE::HELD;
	}

	float InputStateMap::holdingTime(const u32 key)
	{
		if (isInactive(key))
		{
			return 0.f;
		}

		return m_stateMap[key].holdingTime;
	}

	void InputStateMap::updateActiveKeys(const float deltaTime)
	{
		for (u32 activeKey : m_activeKeys)
		{
			auto& state = m_stateMap[activeKey];
			state.state = BUTTON_STATE::HELD;
			state.holdingTime += deltaTime;
			//SDEBUG_LOG("key %u is held for %f\n", activeKey, state.holdingTime);
		}
	}

	const eastl::fixed_set<u32, MULTITOUCH_LIMIT, false>& InputStateMap::activeKeys()
	{
		return m_activeKeys;
	}

	bool InputStateMap::isKeyActive(const u32 key)
	{
		return eastl::find(m_activeKeys.begin(), m_activeKeys.end(), key) != m_activeKeys.end();
	}

	void InputStateMap::setButtonPress(const u32 key)
	{
		if (isInactive(key))
		{
			//SDEBUG_LOG("%u key pressed\n", key);
			ButtonState state(BUTTON_STATE::PRESSED);
			m_stateMap[key] = state;
			m_activeKeys.emplace(key);
			return;
		}
		//SDEBUG_LOG("%u key held\n", key);
		//m_stateMap[key].state = BUTTON_STATE::HELD;
	}

	void InputStateMap::setButtonRelease(const u32 key)
	{
		SASSERTM(!isInactive(key),
		         "Key %u was inactive, it can't be released without previous state",
		         key);
		//SDEBUG_LOG("%u key released\n", key);
		m_activeKeys.erase(key);
		m_stateMap[key].state = BUTTON_STATE::RELEASED;
		m_stateMap[key].holdingTime = 0.f;
	}

	void InputStateMap::reset()
	{
		for (auto& state : m_stateMap)
		{
			if (state.second.state != BUTTON_STATE::RELEASED)
			{
				continue;
			}

			state.second.state = BUTTON_STATE::IDLE;
		}
	}

	void InputStateMap::registerKey(const u32 key)
	{
		ButtonState state;
		m_stateMap.insert(eastl::make_pair(key, state));
	}

	InputBindingAction::InputBindingAction(eastl::string action,
	                                       const INPUT_ACTION_TYPE type,
	                                       const float scale): action(std::move(action)),
	                                                           type(type), scale(scale)
	{
	}

	bool InputBindingAction::operator==(const InputBindingAction& other) const
	{
		return action == other.action && type == other.type;
	}

	bool InputBindingAction::operator!=(const InputBindingAction& other) const
	{
		return action != other.action || type != other.type;
	}

	sizet InputBindingAction::hash::operator()(const InputBindingAction& inputBindingAction) const
	{
		return eastl::hash<eastl::string>()(inputBindingAction.action);
	}

	u32 keycodeFromString(const cstring str)
	{
		// Handle empty string case
		if (!str || str[0] == '\0') return SDLK_UNKNOWN;

		// Convert single character keys (letters, numbers, symbols)
		if (str[1] == '\0')
		{
			char c = str[0];

			// Handle letters (case insensitive)
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
			{
				// Convert to lowercase for SDL keycodes (SDLK_a to SDLK_z)
				char lower = (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
				return static_cast<u32>(lower);
			}

			// Handle numbers and symbols (directly mapped)
			if ((c >= '0' && c <= '9') || (c >= ' ' && c <= '/') || (c >= ':' && c <= '@') || (c >=
				'[' && c <= '`') || (c >= '{' && c <= '~'))
			{
				return static_cast<u32>(c);
			}
		}

		// Handle special keys with string names
		// Function keys
		if (strcmp(str, "F1") == 0) return SDLK_F1;
		if (strcmp(str, "F2") == 0) return SDLK_F2;
		if (strcmp(str, "F3") == 0) return SDLK_F3;
		if (strcmp(str, "F4") == 0) return SDLK_F4;
		if (strcmp(str, "F5") == 0) return SDLK_F5;
		if (strcmp(str, "F6") == 0) return SDLK_F6;
		if (strcmp(str, "F7") == 0) return SDLK_F7;
		if (strcmp(str, "F8") == 0) return SDLK_F8;
		if (strcmp(str, "F9") == 0) return SDLK_F9;
		if (strcmp(str, "F10") == 0) return SDLK_F10;
		if (strcmp(str, "F11") == 0) return SDLK_F11;
		if (strcmp(str, "F12") == 0) return SDLK_F12;

		// Navigation keys
		if (strcmp(str, "UP") == 0 || strcmp(str, "Up") == 0) return SDLK_UP;
		if (strcmp(str, "DOWN") == 0 || strcmp(str, "Down") == 0) return SDLK_DOWN;
		if (strcmp(str, "LEFT") == 0 || strcmp(str, "Left") == 0) return SDLK_LEFT;
		if (strcmp(str, "RIGHT") == 0 || strcmp(str, "Right") == 0) return SDLK_RIGHT;

		// Common control keys
		if (strcmp(str, "RETURN") == 0 || strcmp(str, "Return") == 0 || strcmp(str, "ENTER") == 0 ||
			strcmp(str, "Enter") == 0) return SDLK_RETURN;
		if (strcmp(str, "ESCAPE") == 0 || strcmp(str, "Escape") == 0 || strcmp(str, "ESC") == 0)
			return SDLK_ESCAPE;
		if (strcmp(str, "BACKSPACE") == 0 || strcmp(str, "Backspace") == 0) return SDLK_BACKSPACE;
		if (strcmp(str, "TAB") == 0 || strcmp(str, "Tab") == 0) return SDLK_TAB;
		if (strcmp(str, "SPACE") == 0 || strcmp(str, "Space") == 0) return SDLK_SPACE;
		if (strcmp(str, "DELETE") == 0 || strcmp(str, "Delete") == 0) return SDLK_DELETE;

		// Modifier keys
		if (strcmp(str, "LSHIFT") == 0 || strcmp(str, "LShift") == 0) return SDLK_LSHIFT;
		if (strcmp(str, "RSHIFT") == 0 || strcmp(str, "RShift") == 0) return SDLK_RSHIFT;
		if (strcmp(str, "LCTRL") == 0 || strcmp(str, "LCtrl") == 0) return SDLK_LCTRL;
		if (strcmp(str, "RCTRL") == 0 || strcmp(str, "RCtrl") == 0) return SDLK_RCTRL;
		if (strcmp(str, "LALT") == 0 || strcmp(str, "LAlt") == 0) return SDLK_LALT;
		if (strcmp(str, "RALT") == 0 || strcmp(str, "RAlt") == 0) return SDLK_RALT;

		// Navigation keys
		if (strcmp(str, "HOME") == 0 || strcmp(str, "Home") == 0) return SDLK_HOME;
		if (strcmp(str, "END") == 0 || strcmp(str, "End") == 0) return SDLK_END;
		if (strcmp(str, "PAGEUP") == 0 || strcmp(str, "PageUp") == 0) return SDLK_PAGEUP;
		if (strcmp(str, "PAGEDOWN") == 0 || strcmp(str, "PageDown") == 0) return SDLK_PAGEDOWN;
		if (strcmp(str, "INSERT") == 0 || strcmp(str, "Insert") == 0) return SDLK_INSERT;

		auto unknown = SDLK_UNKNOWN;
		SASSERTM(unknown!= SDLK_UNKNOWN, "Parsed input key string %s is invalid ", str);
		return SDLK_UNKNOWN;
	}

	INPUT_ACTION_TYPE inputActionTypeFromString(const cstring str)
	{
		if (strcmp(str, "PRESS") == 0 || strcmp(str, "Press") == 0) return INPUT_ACTION_TYPE::PRESS;
		if (strcmp(str, "RELEASE") == 0 || strcmp(str, "Release") == 0) return
			INPUT_ACTION_TYPE::RELEASE;
		if (strcmp(str, "HOLD") == 0 || strcmp(str, "Hold") == 0) return INPUT_ACTION_TYPE::HOLD;
		if (strcmp(str, "AXIS") == 0 || strcmp(str, "Axis") == 0) return INPUT_ACTION_TYPE::AXIS;

		auto none = INPUT_ACTION_TYPE::NO_ACTION;
		SASSERTM(none != INPUT_ACTION_TYPE::NO_ACTION,
		         "Parsed input action type string %s is invalid ",
		         str);
		return INPUT_ACTION_TYPE::NO_ACTION;
	}

	InputAction::InputAction(eastl::string name): name(std::move(name))
	{
	}

	InputActionMap::InputActionMap(const cstring inputActionsJsonPath,
	                               const HeapAllocator& allocator): m_bindingActionMap(allocator),
		m_inputActionsMap(allocator), m_activatedActions(allocator)
	{
		InputActionsDeserialized inputActions = parseJson<InputActionsDeserialized>(
			inputActionsJsonPath);
		m_inputActionsCount = inputActions.actions.size();

		m_inputActionsMap.reserve(m_inputActionsCount);
		m_bindingActionMap.reserve(m_inputActionsCount);

		for (const auto& inputAction : inputActions.actions)
		{
			eastl::string actionName(inputAction.action.c_str());
			m_inputActionsMap.emplace(actionName, InputAction(actionName));

			INPUT_ACTION_TYPE type = inputActionTypeFromString(inputAction.type.c_str());

			auto& bindings = inputAction.bindings;
			for (const auto& binding : bindings)
			{
				InputBindingAction inputBindingAction(actionName, type, binding.scale);
				u32 key = keycodeFromString(binding.key.c_str());
				m_bindingActionMap[key].push_back(inputBindingAction);
			}
		}
	}

	void InputActionMap::triggerKeyInteraction(const u32 key,
	                                           const bool isKeyPressed,
	                                           const bool isKeyHeld,
	                                           const float holdingTime)
	{
		auto& bindingActions = m_bindingActionMap[key];

		bool isKeyReleased = !isKeyPressed;

		//bool isKeyPressed = m_inputStateMap->isPressed(key);
		//bool isKeyReleased = m_inputStateMap->isReleased(key);
		//bool isKeyHeld = m_inputStateMap->isHeld(key);
		//float holdingTime = m_inputStateMap->holdingTime(key);

		for (const auto& bindingAction : bindingActions)
		{
			//SDEBUG_LOG("%s action triggered\n", bindingAction.action);
			switch (bindingAction.type)
			{
			case PRESS: if (isKeyPressed && !isKeyHeld)
				{
				//SDEBUG_LOG("KEY %u32 PRESSED\n", key);
					//m_inputActionsMap[bindingAction.action].value = 1.f;
					m_inputActionsMap.at(bindingAction.action).value = 1.f;
				}
				else
				{
				//SDEBUG_LOG("KEY %u32 PRESS INACTIVE\n", key);
					//	m_inputActionsMap[bindingAction.action].value = 0.f;
					m_inputActionsMap.at(bindingAction.action).value = 0.f;
				}
				break;
			case RELEASE: if (isKeyReleased)
				{
					//	m_inputActionsMap[bindingAction.action].value = 1.f;
				//SDEBUG_LOG("KEY %u32 RELEASED\n", key);
					m_inputActionsMap.at(bindingAction.action).value = 1.f;
				}
				else
				{
				//SDEBUG_LOG("KEY %u32 RELEASE INACTIVE\n", key);
					//	m_inputActionsMap[bindingAction.action].value = 0.f;
					m_inputActionsMap.at(bindingAction.action).value = 0.f;
				}
				break;
			//if current holding time >= required holding time for triggering -> trigger action every frame
			case HOLD: if (holdingTime >= bindingAction.scale)
				{
				//SDEBUG_LOG("KEY %u32 HELD\n", key);
					//	m_inputActionsMap[bindingAction.action].value = holdingTime;
					m_inputActionsMap.at(bindingAction.action).value = holdingTime;
				}
				else
				{
				//SDEBUG_LOG("KEY %u32 INACTIVE\n", key);
					//	m_inputActionsMap[bindingAction.action].value = 0.f;
					m_inputActionsMap.at(bindingAction.action).value = 0.f;
				}
				break;
			case AXIS: if (isKeyHeld | isKeyPressed)
				{
				//SDEBUG_LOG("KEY %u32 AXIS ACTIVE\n", key);
					//m_inputActionsMap[bindingAction.action].value = bindingAction.scale;
					m_inputActionsMap.at(bindingAction.action).value = bindingAction.scale;
				}
				else
				{
				//SDEBUG_LOG("KEY %u32 AXIS INACTIVE\n", key);
					m_inputActionsMap.at(bindingAction.action).value = 0.f;
				}
				break;
			default: break;
			}

			m_activatedActions.push_back(m_inputActionsMap.at(bindingAction.action));
		}
	}

	const eastl::vector<InputAction, HeapAllocator>& InputActionMap::activatedActions()
	{
		return m_activatedActions;
	}

	bool InputActionMap::isActionActive(const cstring name)
	{
		return eastl::find(m_activatedActions.begin(), m_activatedActions.end(), InputAction(name)) != m_activatedActions.end();
	}

	float InputActionMap::actionValue(const cstring name)
	{
		SASSERTM(m_inputActionsMap.find(name) != m_inputActionsMap.end(), "Input action %s is not registered! \n", name);
		return m_inputActionsMap.at(name).value;
	}

	void InputActionMap::reset()
	{
		m_activatedActions.clear();
	}

	bool operator==(const InputAction& lhs, const InputAction& rhs)
	{
		return lhs.name == rhs.name;
	}

	bool operator!=(const InputAction& lhs, const InputAction& rhs)
	{
		return !(lhs == rhs);
	}
}
