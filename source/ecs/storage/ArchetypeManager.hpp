#pragma once
#include "Archetype.hpp"

#include "base/CollectionUtilities.hpp"

#include "ecs/core/ComponentMetadataRegistry.hpp"

namespace spite
{
	class AspectRegistry;
	class VersionManager;

	class ArchetypeManager
	{
	private:
		heap_unordered_map<Aspect, std::unique_ptr<Archetype>, Aspect::hash> m_archetypes;
		AspectRegistry* m_aspectRegistry;

		HeapAllocator m_allocator;

		VersionManager* m_versionManager;

		heap_unordered_map<Entity, Archetype*, Entity::hash> m_entityToArchetype;

		DestructionContext m_destructionContext;

	public:
		ArchetypeManager(const HeapAllocator& allocator, AspectRegistry* aspectRegistry,
		                 VersionManager* versionManager, SharedComponentManager* sharedComponentManager);

		ArchetypeManager(const ArchetypeManager&) = delete;
		ArchetypeManager(ArchetypeManager&&) = delete;
		ArchetypeManager& operator =(const ArchetypeManager&) = delete;
		ArchetypeManager& operator =(ArchetypeManager&&) = delete;

		Archetype* getOrCreateArchetype(const Aspect& aspect);

		// (returns nullptr if not found)
		Archetype* findArchetype(const Aspect& aspect) const;

		bool isEntityTracked(Entity entity) const;

		void moveEntity(Entity entity, const Aspect& toAspect);
		void moveEntities(const Aspect& toAspect, eastl::span<const Entity> entities);

		void removeEntity(Entity entity);
		void removeEntities(eastl::span<const Entity> entities);

		void addEntity(const Aspect& aspect, const Entity& entity);
		void addEntities(const Aspect& aspect, eastl::span<const Entity> entities);

		void addComponent(const Entity entity, eastl::span<const ComponentID> componentsToAdd);
		void addComponents(eastl::span<const Entity> entities, eastl::span<const ComponentID> componentsToAdd);

		void removeComponent(const Entity entity, eastl::span<const ComponentID> componentsToRemove);
		void removeComponents(eastl::span<const Entity> entities, eastl::span<const ComponentID> componentsToRemove);

		const Archetype& getEntityArchetype(Entity entity) const;

		const Aspect& getEntityAspect(Entity entity) const;

		// Query archetypes that match certain criteria
		heap_vector<Archetype*> queryArchetypes(const Aspect& includeAspect,
		                                        const Aspect& excludeAspect = {}) const;

		// Query non-empty archetypes that match certain criteria
		heap_vector<Archetype*> queryNonEmptyArchetypes(const Aspect& includeAspect,
		                                                const Aspect& excludeAspect = {}) const;

		//resets component modified statuses of chunks
		void resetAllModificationTracking();

		~ArchetypeManager();

	private:
		//non const helper func
		Archetype& getEntityArchetypeInternal(Entity entity);

		//internal method for managing entity transitions between archetypes (addition/removal of components)
		//true-> removes components
		//false-> adds components
		template <bool ShouldRemove>
		void modifyComponent(const Entity entity, eastl::span<const ComponentID> componentsToModify);

		//internal method for managing entities' transitions in bulk between archetypes (addition/removal of components)
		//true-> removes components
		//false-> adds components
		template <bool ShouldRemove>
		void modifyComponents(eastl::span<const Entity> entities, eastl::span<const ComponentID> componentsToModify);

		void moveEntitiesBetweenArchetypes(Archetype* from, Archetype* to, eastl::span<const Entity> entities);
		//will actually move and destroy components if they're not trivially relocatable
		void copyCompatibleComponents(Chunk* fromChunk,
		                              sizet fromIndex,
		                              Chunk* toChunk,
		                              sizet toIndex,
		                              const Archetype* fromArchetype,
		                              const Archetype* toArchetype) const;
	};

	template <bool ShouldRemove>
	void ArchetypeManager::modifyComponent(const Entity entity, eastl::span<const ComponentID> componentsToModify)
	{
		SASSERT(isEntityTracked(entity))
		Archetype* fromArchetype = m_entityToArchetype.at(entity);

		auto allocMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto components = makeScratchVector<ComponentID>(FrameScratchAllocator::get());

		if constexpr (ShouldRemove)
		{
			for (const auto& id : fromArchetype->aspect().getComponentIds())
			{
				//if should not remove
				if (std::ranges::find(componentsToModify, id) == componentsToModify.end())
				{
					components.push_back(id);
				}
			}
		}
		else
		{
			for (const auto& id : fromArchetype->aspect().getComponentIds())
			{
				components.push_back(id);
			}
			for (const auto& id : componentsToModify)
			{
				components.push_back(id);
			}
		}
		Aspect toAspect(components);
		Archetype* toArchetype = getOrCreateArchetype(toAspect);

		if (toArchetype != fromArchetype)
		{
			moveEntitiesBetweenArchetypes(fromArchetype, toArchetype, {&entity, 1});
		}
	}

	template <bool ShouldRemove>
	void ArchetypeManager::modifyComponents(eastl::span<const Entity> entities,
	                                        eastl::span<const ComponentID> componentsToModify)
	{
		auto allocMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto entityLookup = makeScratchMap<Archetype*, scratch_vector<Entity>>(FrameScratchAllocator::get());

		for (const auto& entity : entities)
		{
			SASSERT(isEntityTracked(entity))
			Archetype* archetype = m_entityToArchetype.at(entity);
			auto it = entityLookup.find(archetype);
			if (it == entityLookup.end())
			{
				it = entityLookup.emplace(archetype, makeScratchVector<Entity>(FrameScratchAllocator::get())).first;
			}
			it->second.push_back(entity);
		}

		for (auto& [fromArchetype, entityGroup] : entityLookup)
		{
			auto toComponents = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
			auto& fromAspect = fromArchetype->aspect();
			//remove or add to aspect
			if constexpr (ShouldRemove)
			{
				for (const auto& componentId : fromAspect.getComponentIds())
				{
					//if componentId should not be removed
					if (std::ranges::find(componentsToModify, componentId) == componentsToModify.end())
					{
						toComponents.push_back(componentId);
					}
				}
			}
			else
			{
				for (const auto& componentId : componentsToModify)
				{
					toComponents.push_back(componentId);
				}
				for (const auto& componentId : fromAspect.getComponentIds())
				{
					toComponents.push_back(componentId);
				}
			}

			Aspect toAspect(toComponents.begin(), toComponents.end());
			Archetype* toArchetype = getOrCreateArchetype(toAspect);

			if (toArchetype != fromArchetype)
			{
				moveEntitiesBetweenArchetypes(fromArchetype, toArchetype, entityGroup);
			}
		}
	}
}
