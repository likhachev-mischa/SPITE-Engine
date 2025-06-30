#include "Query.hpp"
#include "Archetype.hpp"

namespace spite
{
	Query::Query(const ArchetypeManager& archetypeManager, Aspect includeAspect, Aspect excludeAspect)
		: m_includeAspect(std::move(includeAspect)),
		  m_excludeAspect(std::move(excludeAspect))
	{
		SASSERTM(!m_includeAspect.intersects(m_excludeAspect), "Included aspect intersects with excluded aspect!\n")
			m_archetypes = archetypeManager.queryNonEmptyArchetypes(m_includeAspect, m_excludeAspect);
	}

	void Query::rebuild(const ArchetypeManager& archetypeManager)
	{
		m_archetypes = archetypeManager.queryNonEmptyArchetypes(m_includeAspect, m_excludeAspect);
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
