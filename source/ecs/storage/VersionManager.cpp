#include "VersionManager.hpp"
#include "Aspect.hpp"
#include "base/CollectionUtilities.hpp"

namespace spite
{
	VersionManager::VersionManager(const HeapAllocator& allocator,const AspectRegistry* aspectRegistry) : m_versions(makeHeapMap<const Aspect*, u64>(allocator)),m_registry(aspectRegistry) {}

	void VersionManager::makeDirty(const Aspect& aspect)
	{
		const Aspect* canonicalAspect = m_registry->getAspect(aspect);
		if (!canonicalAspect) return;

		const u64 newVersion = m_nextVersion++;

		// Dirty the aspect itself
		m_versions[canonicalAspect] = newVersion;

		// Dirty all of its ancestors in the hierarchy
		scratch_vector<const Aspect*> ancestors = m_registry->getAncestorsAspects(*canonicalAspect);
		for (const Aspect* ancestor : ancestors)
		{
			m_versions[ancestor] = newVersion;
		}
	}

	u64 VersionManager::getVersion(const Aspect& aspect) const
	{
		const Aspect* canonicalAspect = m_registry->getAspect(aspect);
		if (!canonicalAspect) return 0;

		auto it = m_versions.find(canonicalAspect);
		return (it != m_versions.end()) ? it->second : 0;
	}
}
