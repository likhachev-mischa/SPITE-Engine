#pragma once
#include "base/CollectionAliases.hpp"

namespace spite
{
	class Aspect;
	class AspectRegistry;

	class VersionManager 
	{
	private:
		const AspectRegistry* m_registry;
		heap_unordered_map<const Aspect*, u64> m_versions;
		u64 m_nextVersion = 1;

	public:
		VersionManager(const HeapAllocator& allocator, const AspectRegistry* aspectRegistry);

		void makeDirty(const Aspect& aspect);

		u64 getVersion(const Aspect& aspect) const;
	};
}
