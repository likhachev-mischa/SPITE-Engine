#include "AspectRegistry.hpp"

#include "base/CollectionUtilities.hpp"
#include <algorithm>

namespace spite
{
	AspectRegistry::AspectNode::AspectNode(const HeapAllocator& allocator,
	                                       Aspect asp)
		: aspect(std::move(asp)),
		  children(makeHeapVector<AspectNode*>(allocator)),
		  parents(makeHeapVector<AspectNode*>(allocator))
	{
	}

	AspectRegistry::AspectRegistry(const HeapAllocator& allocator): m_allocator(allocator),
	                                                                m_aspectToNode(
		                                                                makeHeapMap<Aspect, AspectNode*, Aspect::hash>(
			                                                                m_allocator))
	{
		m_root = new AspectNode(m_allocator, Aspect());
		m_aspectToNode.emplace(Aspect(), m_root);
	}

	AspectRegistry::~AspectRegistry()
	{
		// With a DAG, recursive deletion is risky. Iterate and delete from the map directly.
		for (auto const& [aspect, node] : m_aspectToNode)
		{
			delete node;
		}
	}

	AspectRegistry::AspectNode* AspectRegistry::addOrGetAspectNode(const Aspect& aspect)
	{
		auto it = m_aspectToNode.find(aspect);
		if (it != m_aspectToNode.end())
		{
			return it->second;
		}

		// 1. Create the new node
		auto newNode = new AspectNode(m_allocator, aspect);
		m_aspectToNode.emplace(aspect, newNode);

		// 2. Find its parents and link them
		auto parents = findBestParents(aspect);
		for (AspectNode* parent : parents)
		{
			newNode->parents.push_back(parent);
			parent->children.push_back(newNode);
		}

		// 3. Find children and reparent them. A child `c` of a parent `p` of `newNode` 
		// might now need to become a child of `newNode` if `c` is a superset of `newNode`.
		for (AspectNode* p : parents)
		{
			auto p_children_copy = makeScratchVector<AspectNode*>(FrameScratchAllocator::get());
			p_children_copy.assign(p->children.begin(), p->children.end());

			for (AspectNode* c : p_children_copy)
			{
				if (c != newNode && c->aspect.contains(newNode->aspect))
				{
					// c should be a child of newNode, and the direct link from p to c is now redundant.
					// Remove p <-> c link
					p->children.erase(std::remove(p->children.begin(), p->children.end(), c), p->children.end());
					c->parents.erase(std::remove(c->parents.begin(), c->parents.end(), p), c->parents.end());

					// Add newNode <-> c link
					newNode->children.push_back(c);
					c->parents.push_back(newNode);
				}
			}
		}

		return newNode;
	}

	scratch_vector<AspectRegistry::AspectNode*> AspectRegistry::findBestParents(const Aspect& newAspect)
	{
		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto candidates = makeScratchVector<AspectNode*>(FrameScratchAllocator::get());

		// 1. Find all proper subsets of newAspect
		for (const auto& [aspect, node] : m_aspectToNode)
		{
			if (newAspect.contains(aspect) && aspect != newAspect)
			{
				candidates.push_back(node);
			}
		}

		auto bestParents = makeScratchVector<AspectNode*>(FrameScratchAllocator::get());

		// 2. Filter for maximal subsets (remove candidates that are subsets of other candidates)
		for (AspectNode* candidate : candidates)
		{
			bool isMaximal = true;
			for (AspectNode* other : candidates)
			{
				if (candidate != other && other->aspect.contains(candidate->aspect))
				{
					isMaximal = false;
					break;
				}
			}
			if (isMaximal)
			{
				bestParents.push_back(candidate);
			}
		}

		// If no subsets were found, the root is the parent.
		if (bestParents.empty() && !newAspect.empty())
		{
			bestParents.push_back(m_root);
		}

		return bestParents;
	}

	scratch_vector<const Aspect*> AspectRegistry::getDescendantAspects(const Aspect& aspect) const
	{
		auto descendants = makeScratchVector<const Aspect*>(FrameScratchAllocator::get());
		AspectNode* node = getNode(aspect);
		if (node)
		{
			auto visited = makeScratchSet<const AspectNode*>(FrameScratchAllocator::get());
			collectDescendants(node, descendants, visited);
		}
		return descendants;
	}

	void AspectRegistry::collectDescendants(AspectNode* node, scratch_vector<const Aspect*>& descendants,
	                                        scratch_set<const AspectNode*>& visited) const
	{
		for (AspectNode* child : node->children)
		{
			if (visited.find(child) == visited.end())
			{
				visited.insert(child);
				descendants.push_back(&child->aspect);
				collectDescendants(child, descendants, visited);
			}
		}
	}

	scratch_vector<const Aspect*> AspectRegistry::getAncestorsAspects(const Aspect& aspect) const
	{
		auto ancestors = makeScratchVector<const Aspect*>(FrameScratchAllocator::get());
		AspectNode* node = getNode(aspect);
		if (node)
		{
			auto visited = makeScratchSet<const AspectNode*>(FrameScratchAllocator::get());
			collectAncestors(node, ancestors, visited);
		}
		return ancestors;
	}

	void AspectRegistry::collectAncestors(AspectNode* node, scratch_vector<const Aspect*>& ancestors,
	                                      scratch_set<const AspectNode*>& visited) const
	{
		for (AspectNode* parent : node->parents)
		{
			if (visited.find(parent) == visited.end())
			{
				visited.insert(parent);
				ancestors.push_back(&parent->aspect);
				collectAncestors(parent, ancestors, visited);
			}
		}
	}

	bool AspectRegistry::removeAspect(const Aspect& aspect)
	{
		if (aspect.empty()) return false; // Cannot remove root

		AspectNode* node = getNode(aspect);
		if (!node) return false;

		// For each child, connect it to all of this node's parents.
		for (AspectNode* child : node->children)
		{
			child->parents.erase(std::remove(child->parents.begin(), child->parents.end(), node), child->parents.end());
			for (AspectNode* parent : node->parents)
			{
				child->parents.push_back(parent);
				parent->children.push_back(child);
			}
		}

		// For each parent, remove this node from its children.
		for (AspectNode* parent : node->parents)
		{
			parent->children.erase(std::remove(parent->children.begin(), parent->children.end(), node),
			                       parent->children.end());
		}

		m_aspectToNode.erase(aspect);
		delete node;
		return true;
	}

	AspectRegistry::AspectNode* AspectRegistry::getNode(const Aspect& aspect) const
	{
		auto it = m_aspectToNode.find(aspect);
		return (it != m_aspectToNode.end()) ? it->second : nullptr;
	}

	heap_vector<AspectRegistry::AspectNode*> AspectRegistry::getParents(const Aspect& aspect) const
	{
		AspectNode* node = getNode(aspect);
		return node ? node->parents : heap_vector<AspectNode*>();
	}

	scratch_vector<const Aspect*> AspectRegistry::findIntersectingAspects(const Aspect& aspect) const
	{
		auto intersecting = makeScratchVector<const Aspect*>(FrameScratchAllocator::get());
		for (const auto& [storedAspect, node] : m_aspectToNode)
		{
			if (storedAspect.intersects(aspect) && storedAspect != aspect)
			{
				intersecting.push_back(&node->aspect);
			}
		}
		return intersecting;
	}

	const Aspect* AspectRegistry::getAspect(const Aspect& aspect) const
	{
		auto it = m_aspectToNode.find(aspect);
		if (it != m_aspectToNode.end())
		{
			return &it->second->aspect;
		}
		return nullptr;
	}

	const Aspect* AspectRegistry::addOrGetAspect(const Aspect& aspect)
	{
		AspectNode* node = addOrGetAspectNode(aspect);
		return node ? &node->aspect : nullptr;
	}

	bool AspectRegistry::hasAspect(const Aspect& aspect) const
	{
		return m_aspectToNode.find(aspect) != m_aspectToNode.end();
	}

	sizet AspectRegistry::size() const
	{
		return m_aspectToNode.size();
	}
}
