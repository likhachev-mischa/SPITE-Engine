#pragma once
#include <base/Platform.hpp>

namespace spite
{
	constexpr sizet INPUT_EVENT_COUNT = 12;

	enum InputEvents
	{
		NONE = 0,
		FRAMEBUFFER_RESIZE = (1u << 0),
		ROTATION_BUTTON_PRESS = (1u << 1),
		SCALING_BUTTON_PRESS = (1u << 2),
		TRANSLATION_BUTTON_PRESS = (1u << 3),
		NEXT_FIGURE_BUTTON_PRESS = (1u << 4),
		FWD_BUTTON_PRESS = (1u<<5),
		BCKWD_BUTTON_RESS = (1u<<6),
		LFT_BUTTON_PRESS = (1u<<7),
		RGHT_BUTTON_PRESS = (1u << 8),
		LOOKUP_BUTTON_PRESS = (1u<<9),
		LOOKDWN_BUTTON_PRESS = (1u<<10),
		LOOKRGHT_BUTTON_PRESS = (1u<<11),
		LOOKLFT_BUTTON_PRESS = (1u << 12)
	};
}
