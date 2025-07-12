#pragma once
#include "Aspect.hpp"
#include "Chunk.hpp"
#include "ecs/core/Entity.hpp"
#include "base/CollectionUtilities.hpp"
#include "base/memory/ScratchAllocator.hpp"

namespace spite
{
	class VersionManager;

	// An Archetype manages a collection of Chunks, all sharing the same Aspect.
	class Archetype
	{
		const Aspect* m_aspect;
		heap_unordered_map<ComponentID, int> m_componentIdToIndexMap;

		const ComponentMetadataRegistry* m_metadataRegistry;

		heap_vector<std::unique_ptr<Chunk>> m_chunks;
		heap_vector<std::unique_ptr<Chunk>> m_freeChunks;
		sizet m_firstNonFullChunkIdx;
		HeapAllocator& m_allocator;

		// Maps Entity ID to its location (chunkIdx, entityIdxInChunk)
		heap_unordered_map<Entity, eastl::pair<sizet, sizet>, Entity::hash> m_entityLocations;

	public:
		Archetype(const Aspect* aspect,
		          const ComponentMetadataRegistry* registry,
		          HeapAllocator& allocator);

		// Finds or creates a chunk with space, adds the entity, and returns its location.
		// second elem in pair is Entity's idx in provided chunk
		// The caller would then emplace/construct components into the returned pointers.
		eastl::pair<Chunk*, sizet> addEntity(const Entity entity);
		scratch_vector<eastl::pair<Chunk*, sizet>> addEntities(eastl::span<const Entity> entities);

		void removeEntity(Entity entity);
		void removeEntities(eastl::span<const Entity> entities, const Aspect* skipDestructionAspect = nullptr);

		const heap_vector<std::unique_ptr<Chunk>>& getChunks() const;

		const Aspect& aspect() const;

		int getComponentIndex(ComponentID id) const;

		eastl::pair<Chunk*, sizet> getEntityLocation(Entity entity) const;

		bool isEmpty() const;
	};

	class ArchetypeManager
	{
	private:
		heap_unordered_map<Aspect, std::unique_ptr<Archetype>, Aspect::hash> m_archetypes;
		AspectRegistry* m_aspectRegistry;

		const ComponentMetadataRegistry* m_metadataRegistry;
		HeapAllocator m_allocator;

		VersionManager* m_versionManager;

		heap_unordered_map<Entity, Archetype*, Entity::hash> m_entityToArchetype;

	public:
		ArchetypeManager(const ComponentMetadataRegistry* registry,
		                 const HeapAllocator& allocator, AspectRegistry* aspectRegistry,
		                 VersionManager* versionManager);

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
			moveEntitiesBetweenArchetypes(fromArchetype, toArchetype, entityGroup);
		}
	}
}