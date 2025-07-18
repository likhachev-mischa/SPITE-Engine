#pragma once

#include <tuple>

#include "engine/components/RenderSettings.hpp" 
#include "engine/components/GameTime.hpp"

namespace spite 
{
    // This is the only place you need to register a new singleton.
    using SingletonComponentList = std::tuple<
        RenderSettings,
        GameTime
    >;
}
