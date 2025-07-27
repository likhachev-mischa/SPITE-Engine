#include "Query.hpp"
#include "ecs/storage/Archetype.hpp"
#include "base/memory/ScratchAllocator.hpp"

namespace spite
{
	Query::Query(const ArchetypeManager* archetypeManager, const Aspect* includeAspect, const Aspect* readAspect,
	             const Aspect* writeAspect,
	             const Aspect* excludeAspect, const Aspect* mustBeEnabledAspect,
	             const Aspect* mustBeModifiedAspect): m_includeAspect(includeAspect),
	                                                  m_readAspect(readAspect),
	                                                  m_writeAspect(writeAspect),
	                                                  m_excludeAspect(excludeAspect),
	                                                  m_mustBeEnabledAspect(
		                                                  mustBeEnabledAspect),
	                                                  m_mustBeModifiedAspect(
		                                                  mustBeModifiedAspect),
	                                                  m_archetypes(archetypeManager->queryNonEmptyArchetypes(
		                                                  *m_includeAspect,
		                                                  *m_excludeAspect))
	{
		SASSERTM(!includeAspect->intersects(*m_excludeAspect),
		         "Included aspect intersects with excluded aspect!\n")
	}

	sizet Query::getEntityCount()
	{
		sizet result = 0;
		for (Archetype* archetype : m_archetypes)
		{
			for (const auto& chunk : archetype->getChunks())
			{
				result += chunk->count();
			}
		}
		return result;
	}

	void Query::rebuild(const ArchetypeManager& archetypeManager)
	{
		m_archetypes = archetypeManager.queryNonEmptyArchetypes(*m_includeAspect, *m_excludeAspect);
	}
}
