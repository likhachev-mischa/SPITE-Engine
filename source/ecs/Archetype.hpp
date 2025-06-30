#pragma once
#include "Chunk.hpp"
#include "Entity.hpp"
#include "Aspect.hpp"
#include "base/memory/ScratchAllocator.hpp"

namespace spite
{
	// An Archetype manages a collection of Chunks, all sharing the same Aspect.
	class Archetype
	{
		const Aspect m_aspect;
		heap_unordered_map<std::type_index, int> m_componentIndexMap;

		const ComponentMetadataRegistry* m_metadataRegistry;

		heap_vector<std::unique_ptr<Chunk>> m_chunks;
		heap_vector<std::unique_ptr<Chunk>> m_freeChunks;
		sizet m_firstNonFullChunkIdx;
		HeapAllocator& m_allocator;

		// Maps Entity ID to its location (chunkIdx, entityIdxInChunk)
		heap_unordered_map<Entity, eastl::pair<sizet, sizet>, Entity::hash> m_entityLocations;

	public:
		Archetype(const Aspect aspect,
		          const ComponentMetadataRegistry* registry,
		          HeapAllocator& allocator);

		// Finds or creates a chunk with space, adds the entity, and returns its location.
		// second elem in pair is Entity's idx in provided chunk
		// The caller would then emplace/construct components into the returned pointers.
		eastl::pair<Chunk*, sizet> addEntity(const Entity entity);
		scratch_vector<eastl::pair<Chunk*, sizet>> addEntities(eastl::span<const Entity> entities);

		void removeEntity(Entity entity);
		void removeEntities(eastl::span<const Entity> entities);

		const heap_vector<std::unique_ptr<Chunk>>& getChunks() const;

		const Aspect& getAspect() const;

		int getComponentIndex(const std::type_index& type) const;

		eastl::pair<Chunk*, sizet> getEntityLocation(Entity entity) const;

		bool isEmpty() const;
	};

	class ArchetypeManager
	{
	private:
		heap_unordered_map<Aspect, std::unique_ptr<Archetype>, Aspect::hash> m_archetypes;
		AspectRegistry m_aspectRegistry;

		const ComponentMetadataRegistry* m_metadataRegistry;
		HeapAllocator m_allocator;

		heap_unordered_map<Entity, Archetype*, Entity::hash> m_entityToArchetype;

	public:
		ArchetypeManager(const ComponentMetadataRegistry* registry,
		                 const HeapAllocator& allocator);

		// Aspect registration should be managed outside
		Archetype* getOrCreateArchetype(const Aspect& aspect);

		// (returns nullptr if not found)
		Archetype* findArchetype(const Aspect& aspect) const;

		bool isEntityTracked(Entity entity) const;

		void moveEntity(Entity entity, const Aspect& fromAspect, const Aspect& toAspect);
		void moveEntities(const Aspect& toAspect, eastl::span<const Entity> entities);

		void removeEntity(Entity entity, const Aspect& aspect);
		void removeEntities(eastl::span<const Entity> entities);

		void addEntities(const Aspect& aspect, eastl::span<const Entity> entities);

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
		void moveEntitiesBetweenArchetypes(Archetype* from, Archetype* to, eastl::span<const Entity> entities);
		void copyCompatibleComponents(Chunk* fromChunk,
		                              sizet fromIndex,
		                              Chunk* toChunk,
		                              sizet toIndex,
		                              const Aspect& fromAspect,
		                              const Aspect& toAspect);
	};
}
