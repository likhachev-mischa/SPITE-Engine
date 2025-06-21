#pragma once
#include "Chunk.hpp"
#include "Entity.hpp"

namespace spite
{
	// An Archetype manages a collection of Chunks, all sharing the same Aspect.
	class Archetype
	{
		const Aspect* m_aspect;

		const ComponentMetadataRegistry* m_metadataRegistry;

		heap_vector<std::unique_ptr<Chunk>> m_chunks;
		HeapAllocator m_allocator;

		// Maps Entity ID to its location (chunkIdx, entityIdxInChunk)
		heap_unordered_map<Entity, eastl::pair<sizet, sizet>, Entity::hash> m_entityLocations;

	public:
		Archetype(const Aspect* aspect,
		          const ComponentMetadataRegistry* registry,
		          const HeapAllocator& allocator);

		// Finds or creates a chunk with space, adds the entity, and returns its location.
		// second elem in pair is Entity's idx in provided chunk
		// The caller would then emplace/construct components into the returned pointers.
		eastl::pair<Chunk*, sizet> addEntity(const Entity entity);

		void removeEntity(Entity entity);

		const heap_vector<std::unique_ptr<Chunk>>& getChunks() const;

		eastl::pair<Chunk*, sizet> getEntityLocation(Entity entity) const;
	};

	class ArchetypeManager
	{
	private:
		heap_unordered_map<Aspect, std::unique_ptr<Archetype>> m_archetypes;
		const ComponentMetadataRegistry* m_metadataRegistry;
		HeapAllocator m_allocator;

		heap_unordered_map<Entity, Archetype*, Entity::hash> m_entityToArchetype;

	public:
		ArchetypeManager(const ComponentMetadataRegistry* registry,
		                 const HeapAllocator& allocator) : m_metadataRegistry(registry),
		                                                   m_allocator(allocator),
		                                                   m_entityToArchetype(allocator)
		{
		}

		// Get or create an archetype for the given aspect
		Archetype* getOrCreateArchetype(const Aspect& aspect);

		// Find an existing archetype (returns nullptr if not found)
		Archetype* findArchetype(const Aspect& aspect) const;

		// Get all archetypes
		const heap_unordered_map<Aspect, std::unique_ptr<Archetype>>& getArchetypes() const;

		bool isEntityTracked(Entity entity) const;

		// Move entity between archetypes (for component add/remove operations)
		void moveEntity(Entity entity, const Aspect& fromAspect, const Aspect& toAspect);

		// Remove entity from its archetype
		void removeEntity(Entity entity, const Aspect& aspect);

		// Get entity's current archetype
		Archetype* getEntityArchetype(Entity entity) const;

		// Query archetypes that match certain criteria
		heap_vector<Archetype*> queryArchetypes(const Aspect& includeAspect,
		                                        const Aspect& excludeAspect = {}) const;

	private:

		void copyCompatibleComponents(Chunk* fromChunk,
		                              sizet fromIndex,
		                              Chunk* toChunk,
		                              sizet toIndex,
		                              const Aspect& fromAspect,
		                              const Aspect& toAspect);
	};
}
