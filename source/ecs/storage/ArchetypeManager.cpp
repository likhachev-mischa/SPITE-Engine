#include "ArchetypeManager.hpp"

#include "AspectRegistry.hpp"
#include "VersionManager.hpp"

#include "base/CollectionUtilities.hpp"
#include "base/memory/ScratchAllocator.hpp"

#include "ecs/core/ComponentMetadataRegistry.hpp"

namespace spite
{
	ArchetypeManager::ArchetypeManager(const HeapAllocator& allocator, AspectRegistry* aspectRegistry,
	                                   VersionManager* versionManager,
	                                   SharedComponentManager* sharedComponentManager) : m_archetypes(
			makeHeapMap<Aspect, std::unique_ptr<Archetype>, Aspect::hash>(allocator)),
		m_aspectRegistry(aspectRegistry),
		m_allocator(allocator),
		m_versionManager(versionManager),
		m_entityToArchetype(
			makeHeapMap<Entity, Archetype*, Entity::hash>(allocator)),
		m_destructionContext(sharedComponentManager)
	{
	}

	Archetype* ArchetypeManager::getOrCreateArchetype(const Aspect& aspect)
	{
		auto it = m_archetypes.find(aspect);
		if (it != m_archetypes.end())
		{
			return it->second.get();
		}

		const Aspect* registeredAspect = m_aspectRegistry->addOrGetAspect(aspect);
		auto newArchetype = std::make_unique<Archetype>(registeredAspect,
		                                                m_allocator);
		Archetype* result = newArchetype.get();
		m_archetypes[aspect] = std::move(newArchetype);

		// A new archetype is created, which is a structural change.
		m_versionManager->makeDirty(*registeredAspect);

		return result;
	}

	Archetype* ArchetypeManager::findArchetype(const Aspect& aspect) const
	{
		auto it = m_archetypes.find(aspect);
		if (it != m_archetypes.end())
		{
			return it->second.get();
		}
		return nullptr;
	}

	void ArchetypeManager::addEntities(const Aspect& aspect, eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;
		Archetype* archetype = getOrCreateArchetype(aspect);

		const bool wasEmpty = archetype->isEmpty();
		archetype->addEntities(entities);
		if (wasEmpty)
		{
			m_versionManager->makeDirty(archetype->aspect());
		}

		for (const auto& entity : entities)
		{
			m_entityToArchetype[entity] = archetype;
		}
	}

	void ArchetypeManager::addComponent(const Entity entity, eastl::span<const ComponentID> componentsToAdd)
	{
		modifyComponent<false>(entity, componentsToAdd);
	}

	void ArchetypeManager::addComponents(eastl::span<const Entity> entities,
	                                     eastl::span<const ComponentID> componentsToAdd)
	{
		modifyComponents<false>(entities, componentsToAdd);
	}

	void ArchetypeManager::removeComponent(const Entity entity, eastl::span<const ComponentID> componentsToRemove)
	{
		modifyComponent<true>(entity, componentsToRemove);
	}

	void ArchetypeManager::removeComponents(eastl::span<const Entity> entities,
	                                        eastl::span<const ComponentID> componentsToRemove)
	{
		modifyComponents<true>(entities, componentsToRemove);
	}

	void ArchetypeManager::moveEntity(Entity entity,
	                                  const Aspect& toAspect)
	{
		SASSERT(isEntityTracked(entity))
		Archetype* fromArchetype = m_entityToArchetype.at(entity);
		Archetype* toArchetype = getOrCreateArchetype(toAspect);

		SASSERT(fromArchetype)
		SASSERT(toArchetype)

		moveEntitiesBetweenArchetypes(fromArchetype, toArchetype, {&entity, 1});
	}

	void ArchetypeManager::moveEntities(const Aspect& toAspect, eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;

		Archetype* toArchetype = getOrCreateArchetype(toAspect);
		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto groups = makeScratchMap<Archetype*, scratch_vector<Entity>>(
			FrameScratchAllocator::get());

		for (const auto& entity : entities)
		{
			SASSERT(isEntityTracked(entity))
			Archetype* archetype = m_entityToArchetype.at(entity);
			auto it = groups.find(archetype);
			if (it == groups.end())
			{
				it = groups.emplace(archetype, makeScratchVector<Entity>(FrameScratchAllocator::get())).first;
			}
			it->second.push_back(entity);
		}

		for (auto const& [fromArchetype, entityGroup] : groups)
		{
			if (fromArchetype != toArchetype)
			{
				moveEntitiesBetweenArchetypes(fromArchetype, toArchetype, entityGroup);
			}
		}
	}

	void ArchetypeManager::removeEntity(Entity entity)
	{
		auto& archetype = getEntityArchetypeInternal(entity);

		const bool wasEmpty = archetype.isEmpty();
		archetype.removeEntity(entity, m_destructionContext);
		const bool isNowEmpty = archetype.isEmpty();

		if (!wasEmpty && isNowEmpty)
		{
			m_versionManager->makeDirty(archetype.aspect());
		}

		m_entityToArchetype.erase(entity);
	}

	void ArchetypeManager::removeEntities(eastl::span<const Entity> entities)
	{
		if (entities.empty()) return;

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto groups = makeScratchMap<Archetype*, scratch_vector<Entity>>(
			FrameScratchAllocator::get());
		for (const auto& entity : entities)
		{
			auto it = m_entityToArchetype.find(entity);
			if (it != m_entityToArchetype.end())
			{
				auto groupIt = groups.find(it->second);
				if (groupIt == groups.end())
				{
					groupIt = groups.emplace(it->second, makeScratchVector<Entity>(FrameScratchAllocator::get())).first;
				}
				groupIt->second.push_back(entity);
			}
		}

		for (auto const& [archetype, entityGroup] : groups)
		{
			const bool wasEmpty = archetype->isEmpty();
			archetype->removeEntities(entityGroup, m_destructionContext);
			const bool isNowEmpty = archetype->isEmpty();

			if (!wasEmpty && isNowEmpty)
			{
				m_versionManager->makeDirty(archetype->aspect());
			}
		}

		for (const auto& entity : entities)
		{
			m_entityToArchetype.erase(entity);
		}
	}

	void ArchetypeManager::addEntity(const Aspect& aspect, const Entity& entity)
	{
		Archetype* archetype = getOrCreateArchetype(aspect);

		const bool wasEmpty = archetype->isEmpty();
		archetype->addEntity(entity);
		if (wasEmpty)
		{
			m_versionManager->makeDirty(archetype->aspect());
		}

		m_entityToArchetype[entity] = archetype;
	}

	const Archetype& ArchetypeManager::getEntityArchetype(Entity entity) const
	{
		SASSERT(isEntityTracked(entity))
		return *m_entityToArchetype.at(entity);
	}

	const Aspect& ArchetypeManager::getEntityAspect(Entity entity) const
	{
		return getEntityArchetype(entity).aspect();
	}

	heap_vector<Archetype*> ArchetypeManager::queryArchetypes(const Aspect& includeAspect,
	                                                          const Aspect& excludeAspect) const
	{
		auto result = makeHeapVector<Archetype*>(m_allocator);
		auto* rootAspect = m_aspectRegistry->getAspect(includeAspect);
		if (!rootAspect)
		{
			return result;
		}

		auto descendants = m_aspectRegistry->getDescendantAspects(includeAspect);
		descendants.push_back(rootAspect);

		for (auto* aspect : descendants)
		{
			if (!aspect->intersects(excludeAspect))
			{
				auto* archetype = findArchetype(*aspect);
				if (archetype)
				{
					result.push_back(archetype);
				}
			}
		}

		return result;
	}

	heap_vector<Archetype*> ArchetypeManager::queryNonEmptyArchetypes(const Aspect& includeAspect,
	                                                                  const Aspect& excludeAspect) const
	{
		auto result = makeHeapVector<Archetype*>(m_allocator);
		auto* rootAspect = m_aspectRegistry->getAspect(includeAspect);
		if (!rootAspect)
		{
			return result;
		}

		auto allocMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto descendants = m_aspectRegistry->getDescendantAspects(includeAspect);
		descendants.push_back(rootAspect);

		for (auto* aspect : descendants)
		{
			if (!aspect->intersects(excludeAspect))
			{
				auto* archetype = findArchetype(*aspect);
				if (archetype && !archetype->isEmpty())
				{
					result.push_back(archetype);
				}
			}
		}

		return result;
	}

	void ArchetypeManager::resetAllModificationTracking()
	{
		for (const auto& [aspect, archetype] : m_archetypes)
		{
			for (const auto& chunk : archetype->getChunks())
			{
				chunk->resetModificationTracking();
			}
		}
	}

	ArchetypeManager::~ArchetypeManager()
	{
		for (auto& [aspect,archetype] : m_archetypes)
		{
			archetype->destroyAllComponents(m_destructionContext);
		}
	}

	Archetype& ArchetypeManager::getEntityArchetypeInternal(Entity entity)
	{
		SASSERT(isEntityTracked(entity))
		return *m_entityToArchetype.at(entity);
	}

	bool ArchetypeManager::isEntityTracked(Entity entity) const
	{
		return m_entityToArchetype.find(entity) != m_entityToArchetype.end();
	}

	void ArchetypeManager::moveEntitiesBetweenArchetypes(Archetype* from,
	                                                     Archetype* to,
	                                                     eastl::span<const Entity> entities)
	{
		const bool fromWasEmpty = from->isEmpty();
		const bool toWasEmpty = to->isEmpty();

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto newLocations = to->addEntities(entities);
		auto newLocationMap = makeScratchMap<Entity, eastl::pair<Chunk*, sizet>, Entity::hash>(
			FrameScratchAllocator::get());
		for (sizet i = 0; i < entities.size(); ++i)
		{
			newLocationMap[entities[i]] = newLocations[i];
		}

		for (const auto& entity : entities)
		{
			auto [fromChunk, fromIndex] = from->getEntityLocation(entity);
			auto [toChunk, toIndex] = newLocationMap.at(entity);
			copyCompatibleComponents(fromChunk,
			                         fromIndex,
			                         toChunk,
			                         toIndex,
			                         from,
			                         to);
		}

		from->removeEntities(entities, m_destructionContext, &to->aspect());

		for (auto entity : entities)
		{
			m_entityToArchetype[entity] = to;
		}

		const bool fromIsNowEmpty = from->isEmpty();
		const bool toIsNowEmpty = to->isEmpty();

		if (!fromWasEmpty && fromIsNowEmpty)
		{
			m_versionManager->makeDirty(from->aspect());
		}
		if (toWasEmpty && !toIsNowEmpty)
		{
			m_versionManager->makeDirty(to->aspect());
		}
	}

	void ArchetypeManager::copyCompatibleComponents(Chunk* fromChunk,
	                                                sizet fromIndex,
	                                                Chunk* toChunk,
	                                                sizet toIndex,
	                                                const Archetype* fromArchetype,
	                                                const Archetype* toArchetype) const
	{
		const auto& commonIds = fromArchetype->aspect().getIntersection(toArchetype->aspect());

		for (const auto& componentId : commonIds)
		{
			const auto& metadata = ComponentMetadataRegistry::getMetadata(componentId);

			const int fromComponentIndex = fromArchetype->getComponentIndex(componentId);
			const int toComponentIndex = toArchetype->getComponentIndex(componentId);

			void* srcPtr = fromChunk->getComponentDataPtrByIndex(fromComponentIndex, fromIndex);
			void* dstPtr = toChunk->getComponentDataPtrByIndex(toComponentIndex, toIndex);

			if (srcPtr && dstPtr)
			{
				metadata.moveAndDestroy(dstPtr, srcPtr);
			}
		}
	}
}
