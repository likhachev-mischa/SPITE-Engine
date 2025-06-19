#pragma once
#include <typeindex>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

#include "base/Collections.hpp"
#include "base/memory/HeapAllocator.hpp"

namespace spite
{	
	struct Aspect
	{
	private:
		sbo_vector<std::type_index> m_types;

	public:		
		Aspect() = default;

		Aspect(std::initializer_list<std::type_index> typeList)
			: m_types(typeList) {
			eastl::sort(m_types.begin(), m_types.end());

			// Manual unique removal
			auto writeIt = m_types.begin();
			for (auto readIt = m_types.begin(); readIt != m_types.end(); ++readIt) {
				if (writeIt == m_types.begin() || *readIt != *(writeIt - 1)) {
					*writeIt = *readIt;
					++writeIt;
				}
			}
			while (m_types.end() != writeIt) {
				m_types.pop_back();
			}
		}

		template<typename Iterator>
		Aspect(Iterator begin, Iterator end) {
			m_types.reserve(eastl::distance(begin, end));
			for (auto it = begin; it != end; ++it) {
				m_types.push_back(*it);
			}
			eastl::sort(m_types.begin(), m_types.end());
			// Manual unique removal
			auto writeIt = m_types.begin();
			for (auto readIt = m_types.begin(); readIt != m_types.end(); ++readIt) {
				if (writeIt == m_types.begin() || *readIt != *(writeIt - 1)) {
					*writeIt = *readIt;
					++writeIt;
				}
			}
			while (m_types.end() != writeIt) {
				m_types.pop_back();
			}
		}

		explicit Aspect(std::type_index type) {
			m_types.push_back(type);
		}

		Aspect(const Aspect& other) = default;
		Aspect(Aspect&& other) noexcept = default;
		Aspect& operator=(const Aspect& other) = default;
		Aspect& operator=(Aspect&& other) noexcept = default;

		bool operator==(const Aspect& other) const {
			return m_types == other.m_types;
		}

		bool operator!=(const Aspect& other) const {
			return !(*this == other);
		}

		bool operator<(const Aspect& other) const {
			return m_types < other.m_types;
		}

		bool operator>(const Aspect& other) const {
			return other < *this;
		}

		bool operator<=(const Aspect& other) const {
			return !(other < *this);
		}

		bool operator>=(const Aspect& other) const {
			return !(*this < other);
		}

		const sbo_vector<std::type_index>& getTypes() const
		{
			return m_types;
		}

		// Check if this aspect contains all types from another aspect (subset check)
		bool contains(const Aspect& other) const {
			// Manual implementation of subset check for sorted vectors
			auto it1 = m_types.begin();
			auto it2 = other.m_types.begin();
			
			while (it1 != m_types.end() && it2 != other.m_types.end()) {
				if (*it1 < *it2) {
					++it1;
				} else if (*it1 == *it2) {
					++it1;
					++it2;
				} else {
					// *it1 > *it2, meaning other has a type that this doesn't have
					return false;
				}
			}
			
			// If we've processed all of other's types, then this contains other
			return it2 == other.m_types.end();
		}

		bool contains(std::type_index type) const {
			return eastl::binary_search(m_types.begin(), m_types.end(), type);
		}

		// Check if this aspect intersects with another (has common types)
		bool intersects(const Aspect& other) const {
			// Use two-pointer technique for sorted vectors
			auto it1 = m_types.begin();
			auto it2 = other.m_types.begin();
			
			while (it1 != m_types.end() && it2 != other.m_types.end()) {
				if (*it1 == *it2) {
					return true;
				}
				if (*it1 < *it2) {
					++it1;
				} else {
					++it2;
				}
			}
			return false;
		}

		size_t size() const {
			return m_types.size();
		}

		bool empty() const {
			return m_types.empty();
		}

		struct hash {
			size_t operator()(const Aspect& aspect) const {
				size_t seed = 0;
				for (const auto& type : aspect.m_types) {
					// Combine hashes using a simple hash combination method
					seed ^= std::hash<std::type_index>{}(type) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}
				return seed;
			}
		};

		~Aspect() = default;
	};

	class AspectRegistry
	{
	private:
		struct AspectNode
		{
			const Aspect* aspect;
			glheap_vector<AspectNode*> children;
			AspectNode* parent;

			explicit AspectNode(const Aspect* asp, AspectNode* par = nullptr) 
				: aspect(asp), parent(par)
			{
			}
		};

		AspectNode* m_root;
		glheap_unordered_map<Aspect, AspectNode*, Aspect::hash> m_aspectToNode;

	public:
		AspectRegistry() 
		{
			auto [it, inserted] = m_aspectToNode.emplace(Aspect(), nullptr);
			m_root = new AspectNode(&it->first);
			it->second = m_root;
		}

		~AspectRegistry() 
		{
			destroyNode(m_root);
		}

		// Non-copyable for now 
		AspectRegistry(const AspectRegistry&) = delete;
		AspectRegistry& operator=(const AspectRegistry&) = delete;

		// Add an aspect to the registry and return the node
		AspectNode* addAspect(const Aspect& aspect)
		{
			// Check if aspect already exists
			auto it = m_aspectToNode.find(aspect);
			if (it != m_aspectToNode.end()) {
				return it->second;
			}

			AspectNode* bestParent = findBestParent(aspect);
			
			auto [mapIt, inserted] = m_aspectToNode.emplace(aspect, nullptr);

			AspectNode* newNode = new AspectNode(&mapIt->first, bestParent);
			mapIt->second = newNode;  

			bestParent->children.push_back(newNode);

			reparentChildren(newNode);

			return newNode;
		}

		// Get the node for an aspect (returns nullptr if not found)
		AspectNode* getNode(const Aspect& aspect) const
		{
			auto it = m_aspectToNode.find(aspect);
			return (it != m_aspectToNode.end()) ? it->second : nullptr;
		}

		// Get all children of an aspect (aspects that contain this aspect)
		glheap_vector<AspectNode*> getChildren(const Aspect& aspect) const
		{
			AspectNode* node = getNode(aspect);
			return node ? node->children : glheap_vector<AspectNode*>();
		}

		// Get parent of an aspect (most specific aspect that contains this one)
		AspectNode* getParent(const Aspect& aspect) const
		{
			AspectNode* node = getNode(aspect);
			return node ? node->parent : nullptr;
		}

		// Get all descendants of an aspect (recursively all children)
		glheap_vector<AspectNode*> getDescendants(const Aspect& aspect) const
		{
			glheap_vector<AspectNode*> descendants;
			AspectNode* node = getNode(aspect);
			if (node) {
				collectDescendants(node, descendants);
			}
			return descendants;
		}

		// Get all ancestors of an aspect (path to root)
		glheap_vector<AspectNode*> getAncestors(const Aspect& aspect) const
		{
			glheap_vector<AspectNode*> ancestors;
			AspectNode* node = getNode(aspect);
			while (node && node->parent) {
				ancestors.push_back(node->parent);
				node = node->parent;
			}
			return ancestors;
		}

		// Remove an aspect from the registry
		bool removeAspect(const Aspect& aspect)
		{
			if (aspect.empty()) {
				return false; // Cannot remove root
			}

			AspectNode* node = getNode(aspect);
			if (!node) {
				return false;
			}

			// Reparent children to this node's parent
			for (AspectNode* child : node->children) {
				child->parent = node->parent;
				if (node->parent) {
					node->parent->children.push_back(child);
				}
			}

			// Remove from parent's children list
			if (node->parent) {
				auto& parentChildren = node->parent->children;
				parentChildren.erase(eastl::remove(parentChildren.begin(), parentChildren.end(), node), 
				                    parentChildren.end());
			}

			// Remove from map and delete
			m_aspectToNode.erase(aspect);
			delete node;
			return true;
		}

		// Get the root node (empty aspect)
		AspectNode* getRoot() const 
		{
			return m_root;
		}

		// Check if an aspect is registered
		bool hasAspect(const Aspect& aspect) const
		{
			return m_aspectToNode.find(aspect) != m_aspectToNode.end();
		}
		// Get count of registered aspects
		size_t size() const
		{
			return m_aspectToNode.size();
		}

		// Find all aspects that intersect with the given aspect (have common components)
		glheap_vector<AspectNode*> findIntersecting(const Aspect& aspect) const
		{
			glheap_vector<AspectNode*> intersecting;
			for (const auto& [storedAspect, node] : m_aspectToNode) {
				if (storedAspect.intersects(aspect) && storedAspect != aspect) {
					intersecting.push_back(node);
				}
			}
			return intersecting;
		}

		// Get all aspects in the registry (for iteration)
		glheap_vector<AspectNode*> getAllAspects() const
		{
			glheap_vector<AspectNode*> allAspects;
			allAspects.reserve(m_aspectToNode.size());
			for (const auto& [aspect, node] : m_aspectToNode) {
				allAspects.push_back(node);
			}
			return allAspects;
		}

	private:
		// Find the best parent for a new aspect (most specific aspect that contains the new one)
		AspectNode* findBestParent(const Aspect& newAspect)
		{
			AspectNode* bestParent = m_root;
			
			// Search for the most specific aspect that contains newAspect
			for (const auto& [aspect, node] : m_aspectToNode) {
				if (aspect.contains(newAspect) && aspect != newAspect) {
					// This aspect contains the new one, check if it's more specific than current best
					if (bestParent->aspect->empty() || bestParent->aspect->contains(aspect)) {
						bestParent = node;
					}
				}
			}
			
			return bestParent;
		}

		// Check if any children of the current best parent should become children of the new node
		void reparentChildren(AspectNode* newNode)
		{
			if (!newNode->parent) return;

			auto& parentChildren = newNode->parent->children;
			glheap_vector<AspectNode*> toReparent;

			// Find children that should be reparented to the new node
			for (AspectNode* child : parentChildren) {
				if (child != newNode && newNode->aspect->contains(*child->aspect)) {
					toReparent.push_back(child);
				}
			}

			// Reparent the children
			for (AspectNode* child : toReparent) {
				// Remove from current parent
				parentChildren.erase(eastl::remove(parentChildren.begin(), parentChildren.end(), child), 
				                    parentChildren.end());
				
				// Add to new parent
				child->parent = newNode;
				newNode->children.push_back(child);
			}
		}

		// Recursively collect all descendants
		void collectDescendants(AspectNode* node, glheap_vector<AspectNode*>& descendants) const
		{
			for (AspectNode* child : node->children) {
				descendants.push_back(child);
				collectDescendants(child, descendants);
			}
		}

		// Recursively destroy a node and all its children
		void destroyNode(AspectNode* node)
		{
			if (!node) return;
			
			for (AspectNode* child : node->children) {
				destroyNode(child);
			}
			delete node;
		}
	};
}
