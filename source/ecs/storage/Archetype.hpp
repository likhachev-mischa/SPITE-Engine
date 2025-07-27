#pragma once
#include "Aspect.hpp"
#include "Chunk.hpp"

#include "ecs/core/Entity.hpp"

namespace spite
{
	// An Archetype manages a collection of Chunks, all sharing the same Aspect.
	class Archetype
	{
		const Aspect* m_aspect;
		heap_unordered_map<ComponentID, int> m_componentIdToIndexMap;

		heap_vector<Chunk*> m_chunks;
		heap_vector<Chunk*> m_freeChunks;
		sizet m_firstNonFullChunkIdx;
		HeapAllocator& m_allocator;

		// Maps Entity ID to its location (chunkIdx, entityIdxInChunk)
		heap_unordered_map<Entity, eastl::pair<sizet, sizet>, Entity::hash> m_entityLocations;

	public:
		Archetype(const Aspect* aspect,
		          HeapAllocator& allocator);

		~Archetype();

		// Finds or creates a chunk with space, adds the entity, and returns its location.
		// second elem in pair is Entity's idx in provided chunk
		// The caller would then emplace/construct components into the returned pointers.
		eastl::pair<Chunk*, sizet> addEntity(const Entity entity);
		scratch_vector<eastl::pair<Chunk*, sizet>> addEntities(eastl::span<const Entity> entities);

		void removeEntity(Entity entity, const DestructionContext& destructionContext);
		void removeEntities(eastl::span<const Entity> entities, const DestructionContext& destructionContext,
		                    const Aspect* skipDestructionAspect = nullptr);

		const heap_vector<Chunk*>& getChunks() const;

		const Aspect& aspect() const;

		int getComponentIndex(ComponentID id) const;

		eastl::pair<Chunk*, sizet> getEntityLocation(Entity entity) const;

		bool isEmpty() const;

		void destroyAllComponentsInChunk(Chunk* chunk, const DestructionContext& destructionContext) const;

		void destroyAllComponents(const DestructionContext& destructionContext) const;
	};

}
