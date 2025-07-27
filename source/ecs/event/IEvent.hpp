#pragma once
#include "ecs/core/IComponent.hpp"

namespace spite
{
    //is used to mark all event entities
    struct EventTag : IComponent{};

    struct IEvent : IComponent {};

    template<typename T>
    concept t_event = std::is_base_of_v<IEvent, T>;
}