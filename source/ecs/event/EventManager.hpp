#pragma once
#include "IEvent.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"

namespace spite
{
    class EventManager
    {
    private:
        EntityManager* m_entityManager;
        CommandBuffer m_commandBuffer;

    public:
        EventManager(EntityManager* entityManager);

        template<t_event T>
        void fire(T&& event = T{});

        //call right in the start of the frame
        void commit();
        //call at the end of the frame
        void cleanup();
    };

    template <t_event T>
    void EventManager::fire(T&& event) 
    {
        Entity e = m_commandBuffer.createEntity();
        m_commandBuffer.addComponent<T>(e, std::forward<T>(event));
    }
}