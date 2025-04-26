#pragma once

#include <SDL3/SDL_keycode.h>

namespace spite
{
    constexpr Uint32 CUSTOM_KEYCODE_MASK = 1u << 29;

    enum CustomKeycodes : SDL_Keycode {
        SDLK_MOUSE_MOVEMENT = CUSTOM_KEYCODE_MASK | 0x0001u,
    };

    inline bool IsCustomKeycode(SDL_Keycode keycode) {
        return (keycode & CUSTOM_KEYCODE_MASK) != 0;
    }
}
