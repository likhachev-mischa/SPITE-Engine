#pragma once
#include "Aspect.hpp"

namespace spite
{
	class AspectRegistry
	{
	private:
		struct AspectNode
		{
			const Aspect aspect;
			heap_vector<AspectNode*> children;
			AspectNode* parent;

			AspectNode(const HeapAllocator& allocator, Aspect asp, AspectNode* par = nullptr);
		};

		AspectNode* m_root;

		HeapAllocator m_allocator;
		heap_unordered_map<Aspect, AspectNode*, Aspect::hash> m_aspectToNode;

	public:
		explicit AspectRegistry(const HeapAllocator& allocator);

		~AspectRegistry();

		// Non-copyable for now 
		AspectRegistry(const AspectRegistry&) = delete;
		AspectRegistry& operator=(const AspectRegistry&) = delete;
		AspectRegistry(AspectRegistry&&) = delete;
		AspectRegistry& operator=(AspectRegistry&&) = delete;

		// Check if an aspect is registered
		bool hasAspect(const Aspect& aspect) const;
		// get size of registered aspects
		sizet size() const;

		// get the registered aspect (returns nullptr if not found)
		const Aspect* getAspect(const Aspect& aspect) const;

		const Aspect* addOrGetAspect(const Aspect& aspect);

		// Remove an aspect from the registry
		bool removeAspect(const Aspect& aspect);

		// get all descendants of an aspect 
		scratch_vector<const Aspect*> getDescendantAspects(const Aspect& aspect) const;
		// get all ancestors of an aspect 
		scratch_vector<const Aspect*> getAncestorsAspects(const Aspect& aspect) const;
		// Find all aspects that intersect with the given aspect (have common components)
		scratch_vector<const Aspect*> findIntersectingAspects(const Aspect& aspect) const;

	private:
		// Add an aspect to the registry and return the node
		AspectNode* addOrGetAspectNode(const Aspect& aspect);

		// get the node for an aspect (returns nullptr if not found)
		AspectNode* getNode(const Aspect& aspect) const;

		// get all children of an aspect (aspects that contain this aspect)
		heap_vector<AspectNode*> getChildren(const Aspect& aspect) const;

		// get parent of an aspect (most specific aspect that contains this one)
		AspectNode* getParent(const Aspect& aspect) const;


		// get all descendants of an aspect (recursively all children)
		scratch_vector<AspectNode*> getDescendantsNodes(const Aspect& aspect) const;

		// get all ancestors of an aspect (path to root)
		scratch_vector<AspectNode*> getAncestorsNodes(const Aspect& aspect) const;

		// get the root node (empty aspect)
		AspectNode* getRoot() const;

		// Find all aspects that intersect with the given aspect (have common components)
		scratch_vector<AspectNode*> findIntersectingNodes(const Aspect& aspect) const;

		// get all aspects in the registry (for iteration)
		scratch_vector<AspectNode*> getAllAspectNodes() const;

		// Find the best parent for a new aspect (most specific aspect that contains the new one)
		AspectNode* findBestParent(const Aspect& newAspect);

		// Check if any children of the current best parent should become children of the new node
		void reparentChildren(AspectNode* newNode);

		void collectDescendants(AspectNode* node, scratch_vector<const Aspect*>& descendants) const;

		// Recursively collect all descendants
		void collectDescendants(AspectNode* node, scratch_vector<AspectNode*>& descendants) const;

		// Recursively destroy a node and all its children
		void destroyNode(AspectNode* node);
	};

}
