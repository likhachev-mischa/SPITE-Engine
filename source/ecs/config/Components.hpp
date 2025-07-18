#pragma once
#include <tuple>

// --- Include all component headers here ---
#include "ecs/core/IComponent.hpp"
#include "engine/components/CoreComponents.hpp"
#include "engine/components/VulkanComponents.hpp"

struct TEMPCOMPONENT : spite::IComponent {};

namespace spite 
{
    // This tuple is the single source of truth for all components in the engine.
    using ComponentList = std::tuple<
        TEMPCOMPONENT,
        TransformComponent,
        CameraDataComponent,
        VulkanMeshComponent,
        MaterialComponent
    >;
}
