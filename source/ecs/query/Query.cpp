#include "Query.hpp"
#include "ecs/storage/Archetype.hpp"

namespace spite
{
	Query::Query(const ArchetypeManager* archetypeManager, const Aspect* includeAspect, const Aspect* excludeAspect,
		const Aspect* mustBeEnabledAspect, const Aspect* mustBeModifiedAspect): 
		m_includeAspect(includeAspect),
		m_excludeAspect(excludeAspect),
		m_mustBeEnabledAspect(
			mustBeEnabledAspect),
		m_mustBeModifiedAspect(
			mustBeModifiedAspect)
	{
		SASSERTM(!m_includeAspect->intersects(*m_excludeAspect),
		         "Included aspect intersects with excluded aspect!\n")
		m_archetypes = archetypeManager->queryNonEmptyArchetypes(
			*m_includeAspect,
			*m_excludeAspect);
	}

	sizet Query::getEntityCount()
	{
		sizet result = 0;
		for (Archetype* archetype : m_archetypes)
		{
			for (const auto & chunk : archetype->getChunks())
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

	bool Query::wasModified() const
	{
		return m_wasModified;
	}

	void Query::resetModificationStatus() const
	{
		m_wasModified = false;
	}
}

