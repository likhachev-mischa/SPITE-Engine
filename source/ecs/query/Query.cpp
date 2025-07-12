#include "Query.hpp"
#include "ecs/storage/Archetype.hpp"

namespace spite
{
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

