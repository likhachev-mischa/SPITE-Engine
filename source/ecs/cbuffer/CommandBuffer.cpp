#include "CommandBuffer.hpp"
#include "ecs/core/EntityManager.hpp"
#include "base/Collections.hpp"
#include <algorithm>

namespace spite
{
	CommandBuffer::CommandBuffer(ArchetypeManager* archetypeManager, const HeapAllocator& allocator)
		: m_archetypeManager(archetypeManager),
		  m_commandBuffer(makeHeapVector<std::byte>(allocator)), m_nextProxyId(0)
	{
	}

	CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
		: m_archetypeManager(other.m_archetypeManager),
		  m_commandBuffer(std::move(other.m_commandBuffer)),
		  m_nextProxyId(other.m_nextProxyId)
	{
		other.m_nextProxyId = 0;
	}

	CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept
	{
		if (this != &other)
		{
			m_archetypeManager = other.m_archetypeManager;
			m_commandBuffer = std::move(other.m_commandBuffer);
			m_nextProxyId = other.m_nextProxyId;
			other.m_nextProxyId = 0;
		}
		return *this;
	}

	void* CommandBuffer::writeCommand(CommandType type, u16 size)
	{
		std::lock_guard<std::mutex> lock(m_bufferMutex);
		const auto offset = m_commandBuffer.size();
		m_commandBuffer.resize(offset + size);
		auto header = reinterpret_cast<CommandHeader*>(m_commandBuffer.data() + offset);
		header->type = type;
		header->size = size;
		return header;
	}

	Entity CommandBuffer::createEntity()
	{
		const u32 proxyId = m_nextProxyId++;
		auto cmd = static_cast<CreateEntityCmd*>(writeCommand(CommandType::eCreateEntity, sizeof(CreateEntityCmd)));
		cmd->proxyId = proxyId;
		return Entity{proxyId, Entity::PROXY_GENERATION};
	}

	void CommandBuffer::destroyEntity(Entity entity)
	{
		auto cmd = static_cast<DestroyEntityCmd*>(writeCommand(CommandType::eDestroyEntity, sizeof(DestroyEntityCmd)));
		cmd->entity = entity;
	}

	void CommandBuffer::commit(EntityManager& entityManager)
	{
		if (m_commandBuffer.empty())
		{
			return;
		}

		auto marker = FrameScratchAllocator::get().get_scoped_marker();

		// --- Pass 1: Decode and Sort ---
		struct DecodedCommand
		{
			Entity entity;
			CommandType type;
			ComponentID componentId; // Only for Add/Remove
			void* componentData; // Only for Add
		};

		auto decodedCmds = makeScratchVector<DecodedCommand>(FrameScratchAllocator::get());
		decodedCmds.reserve(m_commandBuffer.size() / sizeof(CommandHeader)); // Approximate reservation

		const std::byte* cursor = m_commandBuffer.data();
		const std::byte* end = cursor + m_commandBuffer.size();

		while (cursor < end)
		{
			const auto* header = reinterpret_cast<const CommandHeader*>(cursor);
			switch (header->type)
			{
			case CommandType::eCreateEntity:
				{
					const auto* cmd = reinterpret_cast<const CreateEntityCmd*>(header);
					decodedCmds.push_back({
						Entity{cmd->proxyId, Entity::PROXY_GENERATION}, CommandType::eCreateEntity,
						INVALID_COMPONENT_ID, nullptr
					});
					break;
				}
			case CommandType::eDestroyEntity:
				{
					const auto* cmd = reinterpret_cast<const DestroyEntityCmd*>(header);
					decodedCmds.push_back({cmd->entity, CommandType::eDestroyEntity, INVALID_COMPONENT_ID, nullptr});
					break;
				}
			case CommandType::eAddComponent:
				{
					const auto* cmd = reinterpret_cast<const AddComponentCmd*>(header);
					void* data = const_cast<void*>(reinterpret_cast<const void*>(cmd + 1));
					decodedCmds.push_back({cmd->entity, CommandType::eAddComponent, cmd->componentId, data});
					break;
				}
			case CommandType::eRemoveComponent:
				{
					const auto* cmd = reinterpret_cast<const RemoveComponentCmd*>(header);
					decodedCmds.push_back({cmd->entity, CommandType::eRemoveComponent, cmd->componentId, nullptr});
					break;
				}
			}
			cursor += header->size;
		}

		// Sort by entity to process all commands for an entity at once.
		std::ranges::sort(decodedCmds, [](const auto& a, const auto& b)
		{
			return a.entity.id() < b.entity.id();
		});

		// --- Pass 2: Process Sorted Commands ---

		scratch_vector<Entity> proxyToRealEntity = makeScratchVector<Entity>(FrameScratchAllocator::get());
		proxyToRealEntity.resize(m_nextProxyId, Entity::undefined());

		auto creations = makeScratchMap<Aspect, scratch_vector<Entity>, Aspect::hash>(FrameScratchAllocator::get());
		auto moves = makeScratchMap<Aspect, scratch_vector<Entity>, Aspect::hash>(FrameScratchAllocator::get());
		scratch_vector<Entity> deletions = makeScratchVector<Entity>(FrameScratchAllocator::get());

		//for convenient entities->components bulk sort
		auto componentsToAdd = makeScratchMap<Entity, eastl::pair<scratch_vector<ComponentID>, scratch_vector<void*>>,
		                                      Entity::hash>(
			FrameScratchAllocator::get());

		auto componentsToRemove = makeScratchVector<ComponentID>(FrameScratchAllocator::get());

		for (sizet i = 0; i < decodedCmds.size();)
		{
			Entity currentEntity = decodedCmds[i].entity;

			// Find the range of commands for the current entity
			sizet j = i;
			while (j < decodedCmds.size() && decodedCmds[j].entity == currentEntity)
			{
				j++;
			}

			// Process this entity's commands
			bool isCreatedInThisBuffer = false;
			bool isDestroyedInThisBuffer = false;

			auto finalAspectIds = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
			Aspect finalAspect = isProxy(currentEntity) ? Aspect{} : m_archetypeManager->getEntityAspect(currentEntity);

			finalAspectIds.assign(finalAspect.getComponentIds().begin(), finalAspect.getComponentIds().end());

			componentsToRemove.clear();

			for (sizet k = i; k < j; ++k)
			{
				const auto& cmd = decodedCmds[k];
				switch (cmd.type)
				{
				case CommandType::eCreateEntity:
					isCreatedInThisBuffer = true;
					break;
				case CommandType::eDestroyEntity:
					isDestroyedInThisBuffer = true;
					break;
				case CommandType::eAddComponent:
					{
						//if components was recorded for removal
						if (std::ranges::find(componentsToRemove, cmd.componentId) != componentsToRemove.end())
						{
							break;
						}

						finalAspectIds.push_back(cmd.componentId);
						auto it = componentsToAdd.find(currentEntity);
						if (it == componentsToAdd.end())
						{
							it = componentsToAdd.emplace(currentEntity,
							                             eastl::make_pair(
								                             makeScratchVector<ComponentID>(
									                             FrameScratchAllocator::get()),
								                             makeScratchVector<void*>(FrameScratchAllocator::get()))).
							                     first;
						}
						it->second.first.push_back(cmd.componentId);
						it->second.second.push_back(cmd.componentData);
						break;
					}
				case CommandType::eRemoveComponent:
					{
						finalAspectIds.erase(std::ranges::remove(finalAspectIds,
						                                         cmd.componentId).begin(),
						                     finalAspectIds.end());
						//if component is added+removed -> do nothing
						auto it = componentsToAdd.find(currentEntity);
						if (it != componentsToAdd.end())
						{
							auto& components = componentsToAdd.at(currentEntity).first;
							//clean addition command for this component
							if (components.erase_first(cmd.componentId) != components.end())
							{
								break;
							}
						}

						componentsToRemove.push_back(cmd.componentId);

						break;
					}
				}
			}

			finalAspect = Aspect(finalAspectIds);
			if (isDestroyedInThisBuffer)
			{
				if (!isCreatedInThisBuffer)
				{
					auto it = componentsToAdd.find(currentEntity);
					if (it != componentsToAdd.end())
					{
						componentsToAdd.erase(it);
					}
					// Don't delete entities created and destroyed in the same buffer
					deletions.push_back(currentEntity);
				}
			}
			else
			{
				if (isCreatedInThisBuffer)
				{
					auto it = creations.find(finalAspect);
					if (it == creations.end())
					{
						it = creations.emplace(finalAspect,
						                       makeScratchVector<Entity>(FrameScratchAllocator::get())).first;
					}
					it->second.push_back(currentEntity);
				}
				else
				{
					if (m_archetypeManager->getEntityAspect(currentEntity) != finalAspect)
					{
						auto it = moves.find(finalAspect);
						if (it == moves.end())
						{
							it = moves.emplace(finalAspect,
							                   makeScratchVector<Entity>(FrameScratchAllocator::get())).first;
						}
						it->second.push_back(currentEntity);
					}
				}
			}
			i = j; // Move to the next entity
		}

		// --- Pass 3: Execute Batched Structural Changes & Set Data ---

		// Execute creations
		for (auto const& [aspect, proxyEntities] : creations)
		{
			//SDEBUG_LOG("CommandBuffer: Creating %zu entities with aspect size %zu\n", proxyEntities.size(), aspect.size());
			scratch_vector<Entity> realEntities = makeScratchVector<Entity>(FrameScratchAllocator::get());
			entityManager.createEntities(proxyEntities.size(), realEntities, aspect);
			for (sizet i = 0; i < proxyEntities.size(); ++i)
			{
				proxyToRealEntity[getProxyId(proxyEntities[i])] = realEntities[i];
			}
		}

		// Execute moves
		for (auto const& [aspect, entities] : moves)
		{
			//SDEBUG_LOG("CommandBuffer: Moving %zu entities to aspect size %zu\n", entities.size(), aspect.size());
			entityManager.moveEntities(aspect, entities);
		}

		// Set component data for all entities that had components added
		for (auto const& [proxyOrRealEntity, components] : componentsToAdd)
		{
			Entity realEntity = isProxy(proxyOrRealEntity)
				                    ? proxyToRealEntity[getProxyId(proxyOrRealEntity)]
				                    : proxyOrRealEntity;
			if (realEntity == Entity::undefined()) continue;

			auto& componentIds = components.first;
			auto& componentDatas = components.second;
			for (sizet i = 0; i < componentIds.size(); ++i)
			{
				entityManager.setComponentData(realEntity, componentIds[i], componentDatas[i]);
			}
		}

		// Execute deletions
		//if (!deletions.empty())
		//{
		//	SDEBUG_LOG("CommandBuffer: Deleting %zu entities\n", deletions.size());
		//}
		entityManager.destroyEntities(deletions);

		// --- Cleanup ---
		m_commandBuffer.clear();
		m_nextProxyId = 0;
	}

	bool CommandBuffer::isProxy(Entity entity)
	{
		return entity.generation() == Entity::PROXY_GENERATION;
	}

	u32 CommandBuffer::getProxyId(Entity entity)
	{
		return entity.index();
	}
}
