#pragma once

namespace spite
{
	enum Events
	{
		NONE = 0,
		FRAMEBUFFER_RESIZE = (1u << 0),
		ROTATION_BUTTON_PRESS = (1u << 1),
		SCALING_BUTTON_PRESS = (1u << 2),
		TRANSLATION_BUTTON_PRESS = (1u << 3),
		NEXT_FIGURE_BUTTON_PRESS = (1u << 4)
	};
}
