#pragma once
#include <algorithm>

#include "ecs/storage/Aspect.hpp"
#include "Entity.hpp"
#include "SingletonComponentRegistry.hpp"

#include "ecs/storage/ArchetypeManager.hpp"
#include "ecs/storage/SharedComponentManager.hpp"

namespace spite
{
	class AspectRegistry;
	class QueryRegistry;
	class QueryBuilder;

	class EntityManager
	{
	private:
		ArchetypeManager* m_archetypeManager;
		SharedComponentManager* m_sharedComponentManager;
		AspectRegistry* m_aspectRegistry;
		SingletonComponentRegistry* m_singletonComponentRegistry;
		QueryRegistry* m_queryRegistry;
		u64 m_nextEntityId;

	public:
		EntityManager(ArchetypeManager* archetypeManager, SharedComponentManager* sharedComponentManager,
		              SingletonComponentRegistry* singletonComponentRegistry, AspectRegistry* aspectRegistry,
		              QueryRegistry* queryRegistry);

		QueryBuilder getQueryBuilder() const;

		//creates Entity with aspect which it will belong to
		Entity createEntity(const Aspect& aspect = Aspect());

		/*
		@brief Creates a specified number of entities and places them
			into the provided container.
		@tparam TContainer A vector - like container that supports clear()
			, reserve(), and emplace_back().
		@param count The number of entities to create.
		@param outEntities The container to be filled with the new
			Entity IDs.The container will be cleared first.
		@param aspect Aspect to which entities will belong.
			*/
		template <typename TContainer>
		void createEntities(sizet count, TContainer& outputEntities, const Aspect& aspect = Aspect());

		void destroyEntity(Entity entity) const;

		void destroyEntities(eastl::span<const Entity> entities) const;

		template <t_component T, typename... Args>
		void addComponent(Entity entity, Args&&... args);

		template <t_component... Components>
		void addComponent(Entity entity);

		template <t_component... Components>
		void addComponents(eastl::span<const Entity> entities);

		//Warning: does not call default ctors for components
		void addComponents(eastl::span<const Entity> entities, eastl::span<const ComponentID> componentIds) const;

		void moveEntities(const Aspect& toAspect, eastl::span<const Entity> entities) const;

		template <t_component... Components>
		void removeComponent(Entity entity);

		template <t_component... Components>
		void removeComponents(eastl::span<const Entity> entities);

		void removeComponents(eastl::span<const Entity> entities, eastl::span<ComponentID> componentIds) const;

		template <t_component T>
		void enableComponent(Entity entity) const;

		template <t_component T>
		void disableComponent(Entity entity) const;

		template <t_component T>
		T& getComponent(Entity entity);

		template <t_component T>
		const T& getComponent(Entity entity) const;

		template <t_component T>
		bool hasComponent(Entity entity) const;

		bool hasComponent(Entity entity, ComponentID id) const;

		void setComponentData(Entity entity, ComponentID componentId, void* componentData) const;

		template <t_shared_component T>
		void setShared(Entity entity, const T& data = T{});

		template <t_shared_component T>
		const T& getShared(Entity entity);

		template <t_shared_component T>
		T& getMutableShared(Entity entity);

		template <t_singleton_component T>
		T& getSingletonComponent();

		template <t_singleton_component T>
		const T& getSingletonComponent() const;
	};

	template <typename TContainer>
	void EntityManager::createEntities(sizet count, TContainer& outputEntities, const Aspect& aspect)
	{
		outputEntities.clear();
		if (count == 0)
		{
			return;
		}

		outputEntities.reserve(count);

		for (sizet i = 0; i < count; ++i)
		{
			outputEntities.emplace_back(Entity(m_nextEntityId++));
		}

		m_archetypeManager->addEntities(aspect, outputEntities);
	}

	template <t_component T, typename ... Args>
	void EntityManager::addComponent(Entity entity, Args&&... args)
	{
		constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<T>();
		m_archetypeManager->addComponent(entity, {&componentId, 1});

		auto& archetype = m_archetypeManager->getEntityArchetype(entity);
		int componentIndexInChunk = archetype.getComponentIndex(componentId);

		SASSERTM(componentIndexInChunk != -1, "Component not found in archetype after adding it\n")

		auto [chunk, entityIndexInChunk] = archetype.getEntityLocation(entity);
		void* componentData = chunk->getComponentDataPtrByIndex(componentIndexInChunk, entityIndexInChunk);
		new(componentData) T(std::forward<Args>(args)...);
	}

	template <t_component ... Components>
	void EntityManager::addComponent(Entity entity)
	{
		constexpr eastl::array<ComponentID, sizeof...(Components)> componentIds = {
			ComponentMetadataRegistry::getComponentId<Components>()...
		};
		m_archetypeManager->addComponent(entity, componentIds);

		auto& archetype = m_archetypeManager->getEntityArchetype(entity);
		auto [chunk, entityIndexInChunk] = archetype.getEntityLocation(entity);

		//default ctor calls
		(void)std::initializer_list<int>{
			(
				[&]
				{
					constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<Components>();
					const int componentIndexInChunk = archetype.getComponentIndex(componentId);
					SASSERTM(componentIndexInChunk != -1, "Component not found in archetype after adding it\n")
					void* componentData = chunk->getComponentDataPtrByIndex(
						componentIndexInChunk, entityIndexInChunk);
					new(componentData) Components();
				}(), 0
			)...
		};
	}

	template <t_component ... Components>
	void EntityManager::addComponents(eastl::span<const Entity> entities)
	{
		constexpr eastl::array<ComponentID, sizeof...(Components)> componentIds = {
			ComponentMetadataRegistry::getComponentId<Components>()...
		};
		m_archetypeManager->addComponents(entities, componentIds);

		for (const Entity& entity : entities)
		{
			auto& archetype = m_archetypeManager->getEntityArchetype(entity);
			auto [chunk, entityIndexInChunk] = archetype.getEntityLocation(entity);

			//default ctor calls
			(void)std::initializer_list<int>{
				(
					[&]
					{
						constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<Components>();
						const int componentIndexInChunk = archetype.getComponentIndex(componentId);
						SASSERTM(componentIndexInChunk != -1, "Component not found in archetype after adding it\n")
						void* componentData = chunk->getComponentDataPtrByIndex(
							componentIndexInChunk, entityIndexInChunk);
						new(componentData) Components();
					}(), 0
				)...
			};
		}
	}

	template <t_component ... Components>
	void EntityManager::removeComponent(Entity entity)
	{
		constexpr eastl::array<ComponentID, sizeof...(Components)> componentIds = {
			ComponentMetadataRegistry::getComponentId<Components>()...
		};
		m_archetypeManager->removeComponent(entity, componentIds);
	}

	template <t_component ... Components>
	void EntityManager::removeComponents(eastl::span<const Entity> entities)
	{
		constexpr eastl::array<ComponentID, sizeof...(Components)> componentIds = {
			ComponentMetadataRegistry::getComponentId<Components>()...
		};
		m_archetypeManager->removeComponents(entities, componentIds);
	}

	template <t_component T>
	void EntityManager::enableComponent(Entity entity) const
	{
		auto& archetype = m_archetypeManager->getEntityArchetype(entity);
		auto [chunk, index] = archetype.getEntityLocation(entity);
		chunk->enableComponentByIndex(archetype.getComponentIndex(ComponentMetadataRegistry::getComponentId<T>()),
		                              index);
	}

	template <t_component T>
	void EntityManager::disableComponent(Entity entity) const
	{
		auto& archetype = m_archetypeManager->getEntityArchetype(entity);
		auto [chunk, index] = archetype.getEntityLocation(entity);
		chunk->disableComponentByIndex(archetype.getComponentIndex(ComponentMetadataRegistry::getComponentId<T>()),
		                               index);
	}

	template <t_component T>
	T& EntityManager::getComponent(Entity entity)
	{
		const auto& archetype = m_archetypeManager->getEntityArchetype(entity);
		constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<T>();
		const int componentIndexInChunk = archetype.getComponentIndex(componentId);
		SASSERT(componentIndexInChunk != -1)

		auto [chunk, entityIndexInChunk] = archetype.getEntityLocation(entity);
		return *static_cast<T*>(chunk->getComponentDataPtrByIndex(componentIndexInChunk, entityIndexInChunk));
	}

	template <t_component T>
	const T& EntityManager::getComponent(Entity entity) const
	{
		const auto& archetype = m_archetypeManager->getEntityArchetype(entity);
		constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<T>();
		const int componentIndexInChunk = archetype.getComponentIndex(componentId);
		SASSERT(componentIndexInChunk != -1)

		const auto [chunk, entityIndexInChunk] = archetype.getEntityLocation(entity);
		return *static_cast<const T*>(chunk->getComponentDataPtrByIndex(componentIndexInChunk, entityIndexInChunk));
	}

	template <t_component T>
	bool EntityManager::hasComponent(Entity entity) const
	{
		const Archetype& archetype = m_archetypeManager->getEntityArchetype(entity);
		constexpr ComponentID componentId = ComponentMetadataRegistry::getComponentId<T>();
		return archetype.aspect().contains(componentId);
	}

	template <t_shared_component T>
	void EntityManager::setShared(Entity entity, const T& data)
	{
		// This is a data-only update if the component already exists, 
		// or a structural change if it doesn't.
		if (hasComponent<SharedComponent<T>>(entity))
		{
			// Data-only update path
			SharedComponent<T>& handleComponent = getComponent<SharedComponent<T>>(entity);
			SharedComponentHandle oldHandle = handleComponent.handle;

			// get a handle to the new data. This also increments the new handle's ref count.
			SharedComponentHandle newHandle = m_sharedComponentManager->getSharedHandle(data);

			// Only perform the swap if the handles are actually different.
			if (oldHandle != newHandle)
			{
				// Decrement the old handle's ref count and assign the new one.
				m_sharedComponentManager->decrementRef(oldHandle);
				handleComponent.handle = newHandle;
			}
			else
			{
				// The new handle is the same as the old one. Since getSharedHandle incremented
				// the ref count, we need to decrement it to maintain the correct count.
				m_sharedComponentManager->decrementRef(newHandle);
			}
		}
		else
		{
			// Structural change path
			SharedComponentHandle newHandle = m_sharedComponentManager->getSharedHandle(data);
			m_sharedComponentManager->incrementRef(newHandle);
			SharedComponent < T > componentToAdd;
			componentToAdd.handle = newHandle;
			addComponent<SharedComponent<T>>(entity, componentToAdd);
		}
	}

	template <t_shared_component T>
	const T& EntityManager::getShared(Entity entity)
	{
		const SharedComponent<T>& handleComp = getComponent<SharedComponent<T>>(entity);
		return m_sharedComponentManager->get<T>(handleComp.handle);
	}

	template <t_shared_component T>
	T& EntityManager::getMutableShared(Entity entity)
	{
		SharedComponent<T>& handleComp = getComponent<SharedComponent<T>>(entity);
		SharedComponentHandle oldHandle = handleComp.handle;
		T & mutableData = m_sharedComponentManager->getMutable<T>(handleComp.handle);
		if (oldHandle.dataIndex != handleComp.handle.dataIndex)
		{
			m_sharedComponentManager->decrementRef(oldHandle);
			m_sharedComponentManager->incrementRef(handleComp.handle);
		}
		return mutableData;
	}

	template <t_singleton_component T>
	T& EntityManager::getSingletonComponent()
	{
		return m_singletonComponentRegistry->get<T>();
	}

	template <t_singleton_component T>
	const T& EntityManager::getSingletonComponent() const
	{
		return m_singletonComponentRegistry->get<T>();
	}
}
