#include "Aspect.hpp"

namespace spite
{
	Aspect::Aspect() = default;

	Aspect::Aspect(std::initializer_list<std::type_index> typeList): m_types(typeList)
	{
		eastl::sort(m_types.begin(), m_types.end());

		// Manual unique removal
		auto writeIt = m_types.begin();
		for (auto readIt = m_types.begin(); readIt != m_types.end(); ++readIt)
		{
			if (writeIt == m_types.begin() || *readIt != *(writeIt - 1))
			{
				*writeIt = *readIt;
				++writeIt;
			}
		}
		while (m_types.end() != writeIt)
		{
			m_types.pop_back();
		}
	}


	Aspect::Aspect(std::type_index type)
	{
		m_types.push_back(type);
	}

	Aspect::Aspect(const Aspect& other) = default;
	Aspect::Aspect(Aspect&& other) noexcept = default;
	Aspect& Aspect::operator=(const Aspect& other) = default;
	Aspect& Aspect::operator=(Aspect&& other) noexcept = default;

	bool Aspect::operator==(const Aspect& other) const
	{
		return m_types == other.m_types;
	}

	bool Aspect::operator!=(const Aspect& other) const
	{
		return !(*this == other);
	}

	bool Aspect::operator<(const Aspect& other) const
	{
		return m_types < other.m_types;
	}

	bool Aspect::operator>(const Aspect& other) const
	{
		return other < *this;
	}

	bool Aspect::operator<=(const Aspect& other) const
	{
		return !(other < *this);
	}

	bool Aspect::operator>=(const Aspect& other) const
	{
		return !(*this < other);
	}

	const sbo_vector<std::type_index>& Aspect::getTypes() const
	{
		return m_types;
	}

	bool Aspect::contains(const Aspect& other) const
	{
		// Manual implementation of subset check for sorted vectors
		auto it1 = m_types.begin();
		auto it2 = other.m_types.begin();

		while (it1 != m_types.end() && it2 != other.m_types.end())
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
		return it2 == other.m_types.end();
	}

	bool Aspect::contains(std::type_index type) const
	{
		return eastl::binary_search(m_types.begin(), m_types.end(), type);
	}

	bool Aspect::intersects(const Aspect& other) const
	{
		// Use two-pointer technique for sorted vectors
		auto it1 = m_types.begin();
		auto it2 = other.m_types.begin();

		while (it1 != m_types.end() && it2 != other.m_types.end())
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

	sbo_vector<std::type_index> Aspect::getIntersection(const Aspect& other) const
	{
		sbo_vector<std::type_index> result;
		auto it1 = m_types.begin();
		auto it2 = other.m_types.begin();

		while (it1 != m_types.end() && it2 != other.m_types.end())
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

	size_t Aspect::size() const
	{
		return m_types.size();
	}

	bool Aspect::empty() const
	{
		return m_types.empty();
	}

	size_t Aspect::hash::operator()(const Aspect& aspect) const
	{
		size_t seed = 0;
		for (const auto& type : aspect.m_types)
		{
			// Combine hashes using a simple hash combination method
			seed ^= std::hash<std::type_index>{}(type) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}

	Aspect::~Aspect() = default;

	AspectRegistry::AspectNode::AspectNode(Aspect asp, AspectNode* par): aspect(std::move(asp)),
		parent(par)
	{
	}

	AspectRegistry::AspectRegistry()
	{
		m_root = new AspectNode(Aspect());
		m_aspectToNode.emplace(Aspect(), m_root);
	}

	AspectRegistry::~AspectRegistry()
	{
		destroyNode(m_root);
	}

	AspectRegistry::AspectNode* AspectRegistry::addAspect(const Aspect& aspect)
	{
		// Check if aspect already exists
		auto it = m_aspectToNode.find(aspect);
		if (it != m_aspectToNode.end())
		{
			return it->second;
		}

		AspectNode* bestParent = findBestParent(aspect);

		auto newNode = new AspectNode(aspect, bestParent);
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

	glheap_vector<AspectRegistry::AspectNode*> AspectRegistry::getChildren(
		const Aspect& aspect) const
	{
		AspectNode* node = getNode(aspect);
		return node ? node->children : glheap_vector<AspectNode*>();
	}

	AspectRegistry::AspectNode* AspectRegistry::getParent(const Aspect& aspect) const
	{
		AspectNode* node = getNode(aspect);
		return node ? node->parent : nullptr;
	}

	glheap_vector<AspectRegistry::AspectNode*> AspectRegistry::getDescendants(
		const Aspect& aspect) const
	{
		glheap_vector<AspectNode*> descendants;
		AspectNode* node = getNode(aspect);
		if (node)
		{
			collectDescendants(node, descendants);
		}
		return descendants;
	}

	glheap_vector<AspectRegistry::AspectNode*> AspectRegistry::getAncestors(
		const Aspect& aspect) const
	{
		glheap_vector<AspectNode*> ancestors;
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

	glheap_vector<AspectRegistry::AspectNode*> AspectRegistry::findIntersecting(
		const Aspect& aspect) const
	{
		glheap_vector<AspectNode*> intersecting;
		for (const auto& [storedAspect, node] : m_aspectToNode)
		{
			if (storedAspect.intersects(aspect) && storedAspect != aspect)
			{
				intersecting.push_back(node);
			}
		}
		return intersecting;
	}

	glheap_vector<AspectRegistry::AspectNode*> AspectRegistry::getAllAspects() const
	{
		glheap_vector<AspectNode*> allAspects;
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
			if (aspect.contains(newAspect) && aspect != newAspect)
			{
				// This aspect contains the new one, check if it's more specific than current best
				if (bestParent->aspect.empty() || bestParent->aspect.contains(aspect))
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
		glheap_vector<AspectNode*> toReparent;

		// Find children that should be reparented to the new node
		for (AspectNode* child : parentChildren)
		{
			if (child != newNode && newNode->aspect.contains(child->aspect))
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
	                                        glheap_vector<AspectNode*>& descendants) const
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
