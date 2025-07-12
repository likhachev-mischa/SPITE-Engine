#include "Aspect.hpp"

#include "base/CollectionUtilities.hpp"

namespace spite
{
	Aspect::Aspect() = default;

	Aspect::Aspect(std::initializer_list<ComponentID> ids): m_componentIds(ids)
	{
		eastl::sort(m_componentIds.begin(), m_componentIds.end());

		// Manual unique removal
		auto writeIt = m_componentIds.begin();
		for (auto readIt = m_componentIds.begin(); readIt != m_componentIds.end(); ++readIt)
		{
			if (writeIt == m_componentIds.begin() || *readIt != *(writeIt - 1))
			{
				*writeIt = *readIt;
				++writeIt;
			}
		}
		while (m_componentIds.end() != writeIt)
		{
			m_componentIds.pop_back();
		}
	}

	Aspect::Aspect(ComponentID id)
	{
		m_componentIds.push_back(id);
	}

	Aspect::Aspect(eastl::span<const ComponentID> ids)
	{
		m_componentIds.reserve(ids.size());
		for (const auto& id : ids)
		{
			m_componentIds.push_back(id);
		}
		eastl::sort(m_componentIds.begin(), m_componentIds.end());
		// Manual unique removal
		auto writeIt = m_componentIds.begin();
		for (auto readIt = m_componentIds.begin(); readIt != m_componentIds.end(); ++readIt)
		{
			if (writeIt == m_componentIds.begin() || *readIt != *(writeIt - 1))
			{
				*writeIt = *readIt;
				++writeIt;
			}
		}
		while (m_componentIds.end() != writeIt)
		{
			m_componentIds.pop_back();
		}
	}

	Aspect::Aspect(const Aspect& other) = default;
	Aspect::Aspect(Aspect&& other) noexcept = default;
	Aspect& Aspect::operator=(const Aspect& other) = default;
	Aspect& Aspect::operator=(Aspect&& other) noexcept = default;

	bool Aspect::operator==(const Aspect& other) const
	{
		return m_componentIds == other.m_componentIds;
	}

	bool Aspect::operator!=(const Aspect& other) const
	{
		return !(*this == other);
	}

	bool Aspect::operator<(const Aspect& other) const
	{
		return m_componentIds < other.m_componentIds;
	}

	bool Aspect::operator>(const Aspect& other) const
	{
		return other < *this;
	}

	bool Aspect::operator<=(const Aspect& other) const
	{
		return other >= *this;
	}

	bool Aspect::operator>=(const Aspect& other) const
	{
		return !(*this < other);
	}

	const sbo_vector<ComponentID>& Aspect::getComponentIds() const
	{
		return m_componentIds;
	}

	Aspect Aspect::add(eastl::span<const ComponentID> ids) const
	{
		auto allocatorMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto tempIds = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
		for (const auto& id : ids)
		{
			tempIds.push_back(id);
		}
		for (const auto& id : getComponentIds())
		{
			tempIds.push_back(id);
		}
		return Aspect(tempIds);
	}

	Aspect Aspect::remove(eastl::span<const ComponentID> ids) const
	{
		auto allocatorMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto tempIds = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
		for (const auto& id : getComponentIds())
		{
			//if not removed
			if (std::ranges::find(ids, id) == ids.end())
			{
				tempIds.push_back(id);
			}
		}
		return Aspect(tempIds);
	}

	bool Aspect::contains(const Aspect& other) const
	{
		// Manual implementation of subset check for sorted vectors
		auto it1 = m_componentIds.begin();
		auto it2 = other.m_componentIds.begin();

		if (std::distance(it1, m_componentIds.end()) < std::distance(it2, other.m_componentIds.end()))
		{
			return false;
		}

		while (it1 != m_componentIds.end() && it2 != other.m_componentIds.end())
		{
			if (*it1 < *it2)
			{
				++it1;
			}
			else if (*it1 == *it2)
			{
				++it1;
				++it2;
			}
			else
			{
				// *it1 > *it2, meaning other has a type that this doesn't have
				return false;
			}
		}

		// If we've processed all of other's types, then this contains other
		return it2 == other.m_componentIds.end();
	}

	bool Aspect::contains(ComponentID id) const
	{
		return eastl::binary_search(m_componentIds.begin(), m_componentIds.end(), id);
	}

	bool Aspect::intersects(const Aspect& other) const
	{
		// Use two-pointer technique for sorted vectors
		auto it1 = m_componentIds.begin();
		auto it2 = other.m_componentIds.begin();

		while (it1 != m_componentIds.end() && it2 != other.m_componentIds.end())
		{
			if (*it1 == *it2)
			{
				return true;
			}
			if (*it1 < *it2)
			{
				++it1;
			}
			else
			{
				++it2;
			}
		}
		return false;
	}

	scratch_vector<ComponentID> Aspect::getIntersection(const Aspect& other) const
	{
		auto result = makeScratchVector<ComponentID>(FrameScratchAllocator::get());
		auto it1 = m_componentIds.begin();
		auto it2 = other.m_componentIds.begin();

		while (it1 != m_componentIds.end() && it2 != other.m_componentIds.end())
		{
			if (*it1 == *it2)
			{
				result.push_back(*it1);
			}
			if (*it1 < *it2)
			{
				++it1;
			}
			else
			{
				++it2;
			}
		}
		return result;
	}

	sizet Aspect::size() const
	{
		return m_componentIds.size();
	}

	bool Aspect::empty() const
	{
		return m_componentIds.empty();
	}

	sizet Aspect::hash::operator()(const Aspect& aspect) const
	{
		size_t seed = 0;
		for (const auto& id : aspect.m_componentIds)
		{
			// Combine hashes using a simple hash combination method
			seed ^= std::hash<ComponentID>{}(id) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}

	Aspect::~Aspect() = default;

	AspectRegistry::AspectNode::AspectNode(const HeapAllocator& allocator,
	                                       Aspect asp,
	                                       AspectNode* par): aspect(std::move(asp)),
	                                                         children(makeHeapVector<AspectNode*>(
		                                                         allocator)), parent(par)
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
		destroyNode(m_root);
	}

	AspectRegistry::AspectNode* AspectRegistry::addOrGetAspectNode(const Aspect& aspect)
	{
		// Check if aspect already exists
		auto it = m_aspectToNode.find(aspect);
		if (it != m_aspectToNode.end())
		{
			return it->second;
		}

		AspectNode* bestParent = findBestParent(aspect);

		auto newNode = new AspectNode(m_allocator, aspect, bestParent);
		m_aspectToNode.emplace(aspect, newNode);

		bestParent->children.push_back(newNode);

		reparentChildren(newNode);

		return newNode;
	}

	AspectRegistry::AspectNode* AspectRegistry::getNode(const Aspect& aspect) const
	{
		auto it = m_aspectToNode.find(aspect);
		return (it != m_aspectToNode.end()) ? it->second : nullptr;
	}

	heap_vector<AspectRegistry::AspectNode*> AspectRegistry::getChildren(const Aspect& aspect) const
	{
		AspectNode* node = getNode(aspect);
		SASSERT(node)
		return node->children;
	}

	AspectRegistry::AspectNode* AspectRegistry::getParent(const Aspect& aspect) const
	{
		AspectNode* node = getNode(aspect);
		return node ? node->parent : nullptr;
	}

	scratch_vector<AspectRegistry::AspectNode*> AspectRegistry::getDescendantsNodes(
		const Aspect& aspect) const
	{
		auto descendants = makeScratchVector<AspectNode*>(FrameScratchAllocator::get());
		AspectNode* node = getNode(aspect);
		if (node)
		{
			collectDescendants(node, descendants);
		}
		return descendants;
	}

	scratch_vector<AspectRegistry::AspectNode*> AspectRegistry::getAncestorsNodes(
		const Aspect& aspect) const
	{
		auto ancestors = makeScratchVector<AspectNode*>(FrameScratchAllocator::get());
		AspectNode* node = getNode(aspect);
		while (node && node->parent)
		{
			ancestors.push_back(node->parent);
			node = node->parent;
		}
		return ancestors;
	}

	bool AspectRegistry::removeAspect(const Aspect& aspect)
	{
		if (aspect.empty())
		{
			return false; // Cannot remove root
		}

		AspectNode* node = getNode(aspect);
		if (!node)
		{
			return false;
		}

		// Reparent children to this node's parent
		for (AspectNode* child : node->children)
		{
			child->parent = node->parent;
			if (node->parent)
			{
				node->parent->children.push_back(child);
			}
		}

		// Remove from parent's children list
		if (node->parent)
		{
			auto& parentChildren = node->parent->children;
			parentChildren.erase(eastl::remove(parentChildren.begin(), parentChildren.end(), node),
			                     parentChildren.end());
		}

		// Remove from map and delete
		m_aspectToNode.erase(aspect);
		delete node;
		return true;
	}

	scratch_vector<const Aspect*> AspectRegistry::getDescendantAspects(const Aspect& aspect) const
	{
		auto descendants = makeScratchVector<const Aspect*>(FrameScratchAllocator::get());
		AspectNode* node = getNode(aspect);
		if (node)
		{
			collectDescendants(node, descendants);
		}
		return descendants;
	}

	scratch_vector<const Aspect*> AspectRegistry::getAncestorsAspects(const Aspect& aspect) const
	{
		auto ancestors = makeScratchVector<const Aspect*>(FrameScratchAllocator::get());
		AspectNode* node = getNode(aspect);
		while (node && node->parent)
		{
			ancestors.push_back(&node->parent->aspect);
			node = node->parent;
		}
		return ancestors;
	}

	scratch_vector<const Aspect*> AspectRegistry::findIntersectingAspects(
		const Aspect& aspect) const
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

	AspectRegistry::AspectNode* AspectRegistry::getRoot() const
	{
		return m_root;
	}

	bool AspectRegistry::hasAspect(const Aspect& aspect) const
	{
		return m_aspectToNode.find(aspect) != m_aspectToNode.end();
	}

	size_t AspectRegistry::size() const
	{
		return m_aspectToNode.size();
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

	scratch_vector<AspectRegistry::AspectNode*> AspectRegistry::findIntersectingNodes(
		const Aspect& aspect) const
	{
		auto intersecting = makeScratchVector<AspectNode*>(FrameScratchAllocator::get());
		for (const auto& [storedAspect, node] : m_aspectToNode)
		{
			if (storedAspect.intersects(aspect) && storedAspect != aspect)
			{
				intersecting.push_back(node);
			}
		}
		return intersecting;
	}

	scratch_vector<AspectRegistry::AspectNode*> AspectRegistry::getAllAspectNodes() const
	{
		auto allAspects = makeScratchVector<AspectNode*>(FrameScratchAllocator::get());
		allAspects.reserve(m_aspectToNode.size());
		for (const auto& [aspect, node] : m_aspectToNode)
		{
			allAspects.push_back(node);
		}
		return allAspects;
	}

	AspectRegistry::AspectNode* AspectRegistry::findBestParent(const Aspect& newAspect)
	{
		AspectNode* bestParent = m_root;

		// Search for the most specific aspect that contains newAspect
		for (const auto& [aspect, node] : m_aspectToNode)
		{
			if (newAspect.contains(aspect) && aspect != newAspect)
			{
				// This aspect contains the new one, check if it's more specific than current best
				if (bestParent->aspect.empty() || bestParent->aspect.size() <= aspect.size())
				{
					bestParent = node;
				}
			}
		}

		return bestParent;
	}

	void AspectRegistry::reparentChildren(AspectNode* newNode)
	{
		if (!newNode->parent) return;

		auto& parentChildren = newNode->parent->children;
		auto allocatorMarker = FrameScratchAllocator::get().get_scoped_marker();
		auto toReparent = makeScratchVector<AspectNode*>(FrameScratchAllocator::get());

		// Find children that should be reparented to the new node
		for (AspectNode* child : parentChildren)
		{
			if (child != newNode && child->aspect.contains(newNode->aspect))
			{
				toReparent.push_back(child);
			}
		}

		// Reparent the children
		for (AspectNode* child : toReparent)
		{
			// Remove from current parent
			parentChildren.erase(eastl::remove(parentChildren.begin(), parentChildren.end(), child),
			                     parentChildren.end());

			// Add to new parent
			child->parent = newNode;
			newNode->children.push_back(child);
		}
	}

	void AspectRegistry::collectDescendants(AspectNode* node,
	                                        scratch_vector<const Aspect*>& descendants) const
	{
		for (AspectNode* child : node->children)
		{
			descendants.push_back(&child->aspect);
			collectDescendants(child, descendants);
		}
	}

	void AspectRegistry::collectDescendants(AspectNode* node,
	                                        scratch_vector<AspectNode*>& descendants) const
	{
		for (AspectNode* child : node->children)
		{
			descendants.push_back(child);
			collectDescendants(child, descendants);
		}
	}

	void AspectRegistry::destroyNode(AspectNode* node)
	{
		if (!node) return;

		for (AspectNode* child : node->children)
		{
			destroyNode(child);
		}
		delete node;
	}
}
