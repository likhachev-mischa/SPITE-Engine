#pragma once
#include <typeindex>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

#include "IComponent.hpp"

#include "base/Collections.hpp"
#include "base/memory/HeapAllocator.hpp"

namespace spite
{
	struct Aspect
	{
	private:
		sbo_vector<std::type_index> m_types;

	public:
		Aspect();

		template <t_component... ComponentTypes>
		Aspect();

		Aspect(std::initializer_list<std::type_index> typeList);

		template <typename Iterator>
		Aspect(Iterator begin, Iterator end);

		explicit Aspect(std::type_index type);

		Aspect(const Aspect& other);
		Aspect(Aspect&& other) noexcept;
		Aspect& operator=(const Aspect& other);
		Aspect& operator=(Aspect&& other) noexcept;

		bool operator==(const Aspect& other) const;

		bool operator!=(const Aspect& other) const;

		bool operator<(const Aspect& other) const;

		bool operator>(const Aspect& other) const;

		bool operator<=(const Aspect& other) const;

		bool operator>=(const Aspect& other) const;

		const sbo_vector<std::type_index>& getTypes() const;

		// Check if this aspect contains all types from another aspect (subset check)
		bool contains(const Aspect& other) const;

		bool contains(std::type_index type) const;

		// Check if this aspect intersects with another (has common types)
		bool intersects(const Aspect& other) const;

		// get all common types
		sbo_vector<std::type_index> getIntersection(const Aspect& other) const;

		size_t size() const;

		bool empty() const;

		struct hash
		{
			size_t operator()(const Aspect& aspect) const;
		};

		~Aspect();
	};

	template <t_component... ComponentTypes>
	Aspect::Aspect()
	{
		Aspect({std::type_index(typeid(ComponentTypes))...});
	}

	template <typename Iterator>
	Aspect::Aspect(Iterator begin, Iterator end)
	{
		m_types.reserve(eastl::distance(begin, end));
		for (auto it = begin; it != end; ++it)
		{
			m_types.push_back(*it);
		}
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

	class AspectRegistry
	{
	private:
		struct AspectNode
		{
			const Aspect aspect;
			glheap_vector<AspectNode*> children;
			AspectNode* parent;

			explicit AspectNode(Aspect asp, AspectNode* par = nullptr);
		};

		AspectNode* m_root;

		glheap_unordered_map<Aspect, AspectNode*, Aspect::hash> m_aspectToNode;

	public:
		AspectRegistry();

		~AspectRegistry();

		// Non-copyable for now 
		AspectRegistry(const AspectRegistry&) = delete;
		AspectRegistry& operator=(const AspectRegistry&) = delete;

		// Add an aspect to the registry and return the node
		AspectNode* addAspect(const Aspect& aspect);

		// Get the node for an aspect (returns nullptr if not found)
		AspectNode* getNode(const Aspect& aspect) const;

		// Get all children of an aspect (aspects that contain this aspect)
		glheap_vector<AspectNode*> getChildren(const Aspect& aspect) const;

		// Get parent of an aspect (most specific aspect that contains this one)
		AspectNode* getParent(const Aspect& aspect) const;

		// Get all descendants of an aspect (recursively all children)
		glheap_vector<AspectNode*> getDescendants(const Aspect& aspect) const;

		// Get all ancestors of an aspect (path to root)
		glheap_vector<AspectNode*> getAncestors(const Aspect& aspect) const;

		// Remove an aspect from the registry
		bool removeAspect(const Aspect& aspect);

		// Get the root node (empty aspect)
		AspectNode* getRoot() const;

		// Check if an aspect is registered
		bool hasAspect(const Aspect& aspect) const;
		// Get count of registered aspects
		size_t size() const;

		// Find all aspects that intersect with the given aspect (have common components)
		glheap_vector<AspectNode*> findIntersecting(const Aspect& aspect) const;

		// Get all aspects in the registry (for iteration)
		glheap_vector<AspectNode*> getAllAspects() const;

	private:
		// Find the best parent for a new aspect (most specific aspect that contains the new one)
		AspectNode* findBestParent(const Aspect& newAspect);

		// Check if any children of the current best parent should become children of the new node
		void reparentChildren(AspectNode* newNode);

		// Recursively collect all descendants
		void collectDescendants(AspectNode* node, glheap_vector<AspectNode*>& descendants) const;

		// Recursively destroy a node and all its children
		void destroyNode(AspectNode* node);
	};
}
