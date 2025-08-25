#pragma once
#include "Aspect.hpp"
#include "base/CollectionUtilities.hpp"

namespace spite
{
	class AspectRegistry
	{
	private:
		struct AspectNode
		{
			const Aspect aspect;
			heap_vector<AspectNode*> children;
			heap_vector<AspectNode*> parents; // Changed to support multiple parents (DAG)

			AspectNode(const HeapAllocator& allocator, Aspect asp);
		};

		AspectNode* m_root;

		HeapAllocator m_allocator;
		heap_unordered_map<Aspect, AspectNode*, Aspect::hash> m_aspectToNode;

	public:
		explicit AspectRegistry(const HeapAllocator& allocator);
		~AspectRegistry();

		// Non-copyable
		AspectRegistry(const AspectRegistry&) = delete;
		AspectRegistry& operator=(const AspectRegistry&) = delete;
		AspectRegistry(AspectRegistry&&) = delete;
		AspectRegistry& operator=(AspectRegistry&&) = delete;

		bool hasAspect(const Aspect& aspect) const;
		sizet size() const;
		const Aspect* getAspect(const Aspect& aspect) const;
		const Aspect* addOrGetAspect(const Aspect& aspect);
		bool removeAspect(const Aspect& aspect);

		scratch_vector<const Aspect*> getDescendantAspects(const Aspect& aspect) const;
		scratch_vector<const Aspect*> getAncestorsAspects(const Aspect& aspect) const;
		scratch_vector<const Aspect*> findIntersectingAspects(const Aspect& aspect) const;

	private:
		AspectNode* addOrGetAspectNode(const Aspect& aspect);
		AspectNode* getNode(const Aspect& aspect) const;

		heap_vector<AspectNode*> getParents(const Aspect& aspect) const;

		// Finds all most-specific parents for a new aspect
		scratch_vector<AspectNode*> findBestParents(const Aspect& newAspect);

		// Traversal helpers with visited set for DAGs
		void collectDescendants(AspectNode* node, scratch_vector<const Aspect*>& descendants,
		                        scratch_set<const AspectNode*>& visited) const;
		void collectAncestors(AspectNode* node, scratch_vector<const Aspect*>& ancestors,
		                      scratch_set<const AspectNode*>& visited) const;
	};
}
