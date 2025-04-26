#pragma once
#include <memory>

#include "base/Platform.hpp"

namespace spite
{
	class InputStateMap;
	class InputActionMap;

	struct MousePosition
	{
		float x;
		float y;

		float xRel;
		float yRel;
	};

	class InputManager
	{
		std::shared_ptr<InputStateMap> m_inputStateMap;
		std::shared_ptr<InputActionMap> m_inputActionMap;

		MousePosition m_mousePosition;

	public:
		InputManager(std::shared_ptr<InputActionMap> inputActionMap);

		//constexpr static sizet MAPPED_KEYS_COUNT = 11;

		//InputManager();

		////return NONE if key is unmapped//invalid
		//InputEvents tryGetEvent(u16 key);

		//InputEvents getEvent(u16 key);

		//bool isKeyMapped(u16 key);

		void update(float deltaTime);

		std::shared_ptr<InputStateMap> inputStateMap();

		std::shared_ptr<InputActionMap> inputActionMap();

		void setMousePosition(const float x, const float y, const float xRel, const float yRel);

		[[nodiscard]] const MousePosition& mousePosition() const;

		void triggerKeyInteraction(const u32 key,const bool isPressed);

		void reset();

	private:
		//	eastl::fixed_map<u16, InputEvents, MAPPED_KEYS_COUNT, false> m_keymap;
	};
}
