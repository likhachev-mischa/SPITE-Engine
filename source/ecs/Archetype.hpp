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

	// An ArchetypeManager 
	// map Aspect -> Archetype
	// class ArchetypeManager {
	// };
}
