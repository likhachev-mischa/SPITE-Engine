#pragma once

#include "ecs/storage/Archetype.hpp"
#include "ecs/core/Entity.hpp"
#include "ecs/core/ComponentMetadataRegistry.hpp"
#include "base/memory/ScratchAllocator.hpp"
#include "base/CollectionAliases.hpp"

namespace spite
{
	class ArchetypeManager;
	class EntityManager;

	// A command buffer for recording entity and component operations to be executed later.
	// Uses a scratch allocator for fast, temporary allocations.
	class CommandBuffer
	{
	private:
		ArchetypeManager& m_archetypeManager;

		enum class CommandType : u8
		{
			eCreateEntity,
			eDestroyEntity,
			eAddComponent,
			eRemoveComponent
		};

		struct CommandHeader
		{
			CommandType type;
			u16 size; // Size of the entire command packet (header + data)
		};

		struct CreateEntityCmd
		{
			CommandHeader header;
			u32 proxyId;
		};

		struct DestroyEntityCmd
		{
			CommandHeader header;
			Entity entity;
		};

		struct AddComponentCmd
		{
			CommandHeader header;
			Entity entity;
			ComponentID componentId;
			// Component data follows immediately in the buffer
		};

		struct RemoveComponentCmd
		{
			CommandHeader header;
			Entity entity;
			ComponentID componentId;
		};

		ScratchAllocator& m_allocator;
		scratch_vector<std::byte> m_commandBuffer;
		u32 m_nextProxyId;

		void* writeCommand(CommandType type, u16 size);

		static bool isProxy(Entity entity);
		static u32 getProxyId(Entity entity);

	public:
		CommandBuffer(ScratchAllocator& allocator, ArchetypeManager& archetypeManager);

		// Creates a proxy entity and records the creation command.
		// Returns a temporary handle to be used in subsequent commands within this buffer.
		Entity createEntity();

		// Records a command to destroy an entity.
		void destroyEntity(Entity entity);

		// Records a command to add a component to an entity.
		template <t_component T>
		void addComponent(Entity entity, T&& component);

		// Records a command to remove a component from an entity.
		template <t_component T>
		void removeComponent(Entity entity);

		// Executes all recorded commands on the EntityManager.
		void commit(EntityManager& entityManager);
	};

	template <t_component T>
	void CommandBuffer::addComponent(Entity entity, T&& component)
	{
		constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<T>();
		const u16 commandSize = sizeof(AddComponentCmd) + sizeof(T);

		auto cmd = static_cast<AddComponentCmd*>(writeCommand(CommandType::eAddComponent, commandSize));
		cmd->entity = entity;
		cmd->componentId = componentId;

		// Write component data directly after the command struct
		new(cmd + 1) T(std::forward<T>(component));
	}

	template <t_component T>
	void CommandBuffer::removeComponent(Entity entity)
	{
		constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<T>();
		const u16 commandSize = sizeof(RemoveComponentCmd);

		auto cmd = static_cast<RemoveComponentCmd*>(writeCommand(CommandType::eRemoveComponent, commandSize));
		cmd->entity = entity;
		cmd->componentId = componentId;
	}
}
