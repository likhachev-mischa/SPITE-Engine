#pragma once

#include <string>
#include <vector>

#include <EASTL/fixed_map.h>
#include <EASTL/hash_map.h>
#include <eastl/string.h>
#include <EASTL/vector.h>

#include <nlohmann/json.hpp>

#include <SDL3/SDL_keycode.h>

#include "Base/Assert.hpp"
#include "base/Memory.hpp"
#include "base/Platform.hpp"

namespace spite
{
	//for json deserializing only
	struct InputBindingDeserialized
	{
		//only keyboard supported for now
		std::string device{};
		std::string key;

		//for axis - the value of an axis
		//for holding - desired time in seconds
		float scale{};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(InputBindingDeserialized, device, key, scale)
	};

	//for json deserializing only
	struct InputActionDeserialized
	{
		std::string action;
		std::string type;
		std::vector<InputBindingDeserialized> bindings;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(InputActionDeserialized, action, type, bindings)
	};

	struct InputActionsDeserialized
	{
		std::vector<InputActionDeserialized> actions;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(InputActionsDeserialized, actions)
	};

	enum INPUT_ACTION_TYPE
	{
		NO_ACTION,
		PRESS,
		RELEASE,
		HOLD,
		AXIS
	};

	enum INPUT_DEVICE
	{
		KEYBOARD,
		GAMEPAD
	};

	enum BUTTON_STATE
	{
		IDLE,
		PRESSED,
		RELEASED,
		HELD
	};

	struct ButtonState
	{
		BUTTON_STATE state;
		float holdingTime{};

		ButtonState();

		explicit ButtonState(const BUTTON_STATE state, const float holdingTime = 0.f);
	};

	//total number of keys to keep track of
	constexpr sizet MAX_KEYS_FOR_PROCESSING = 255;

	constexpr sizet MULTITOUCH_LIMIT = 10;

	//lookup for state of input keys
	class InputStateMap
	{
		eastl::fixed_map<u32, ButtonState, MAX_KEYS_FOR_PROCESSING, false> m_stateMap;

		//held keys
		eastl::fixed_set<u32, MULTITOUCH_LIMIT, false> m_activeKeys;

	public:
		InputStateMap() = default;
		//if key wasn't interacted with for a frame
		bool isInactive(const u32 key);

		//ONLY returns true when key was pressed THIS FRAME
		//returns false if key is held
		bool isPressed(const u32 key);

		bool isReleased(const u32 key);

		bool isHeld(const u32 key);

		float holdingTime(const u32 key);

		void updateActiveKeys(const float deltaTime);

		const eastl::fixed_set<u32, MULTITOUCH_LIMIT, false>& activeKeys();

		bool isKeyActive(const u32 key);

		void setButtonPress(const u32 key);

		void setButtonRelease(const u32 key);

		//should be called every frame before input processing
		void reset();

	private:
		void registerKey(const u32 key);
	};

	//represents connection between input and action
	//used in lookup and accessed by u32 keycode
	struct InputBindingAction
	{
		eastl::string action;
		INPUT_ACTION_TYPE type;

		//for axis - the value of an axis 
		//for holding - desired time in seconds
		const float scale{};

		InputBindingAction(eastl::string action, const INPUT_ACTION_TYPE type, const float scale);

		bool operator ==(const InputBindingAction& other) const;

		bool operator !=(const InputBindingAction& other) const;

		struct hash
		{
			sizet operator()(const InputBindingAction& inputBindingAction) const;
		};
	};

	u32 keycodeFromString(const cstring str);

	INPUT_ACTION_TYPE inputActionTypeFromString(const cstring str);

	//used in inputActionLookup -> is accessed by action name string, holds CURRENT value for this action  
	struct InputAction
	{
		friend bool operator==(const InputAction& lhs, const InputAction& rhs);

		friend bool operator!=(const InputAction& lhs, const InputAction& rhs);

		//name string duplication
		eastl::string name;
		//for binary(press/release) 1.0 - activated, 0.0 - inactive
		//for axis - the value of an axis 
		//for holding - time in seconds
		float value{};

		explicit InputAction(eastl::string name);


	};

	class InputActionMap
	{
		sizet m_inputActionsCount{};
		//fixed
		eastl::hash_map<u32, std::vector<InputBindingAction>, eastl::hash<u32>, eastl::equal_to<u32>
		                , HeapAllocator> m_bindingActionMap;
		//fixed
		eastl::hash_map<eastl::string, InputAction, eastl::hash<eastl::string>, eastl::equal_to<
			                eastl::string>, HeapAllocator> m_inputActionsMap;

		//actions activated this frame
		eastl::vector<InputAction, HeapAllocator> m_activatedActions;

	public:
		InputActionMap(const cstring inputActionsJsonPath, const HeapAllocator& allocator);

		void triggerKeyInteraction(const u32 key,
		                           const bool isKeyPressed,
		                           const bool isKeyHeld,
		                           const float holdingTime);

		const eastl::vector<InputAction, HeapAllocator>& activatedActions();

		bool isActionActive(const cstring name);

		float actionValue (const cstring name);

		//call this every frame before input processing
		void reset();
	};
}
