#pragma once
#include <typeindex>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/span.h>

#include "ecs/core/IComponent.hpp"
#include "ecs/core/ComponentMetadata.hpp"

#include "base/CollectionAliases.hpp"
#include "base/Collections.hpp"
#include "base/memory/HeapAllocator.hpp"

namespace spite
{
	class Aspect
	{
	private:
		sbo_vector<ComponentID> m_componentIds;

	public:
		Aspect();

		Aspect(std::initializer_list<ComponentID> ids);

		template <typename Iterator>
		Aspect(Iterator begin, Iterator end) requires std::is_base_of_v<
			std::forward_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>;

		explicit Aspect(ComponentID id);

		explicit Aspect(eastl::span<const ComponentID> ids);

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

		const sbo_vector<ComponentID>& getComponentIds() const;

		//returns new aspect
		Aspect add(eastl::span<const ComponentID> ids) const;

		//returns new aspect
		Aspect remove(eastl::span<const ComponentID> ids) const;

		// Check if this aspect contains all types from another aspect (subset check)
		bool contains(const Aspect& other) const;

		bool contains(ComponentID id) const;

		// Check if this aspect intersects with another (has common types)
		bool intersects(const Aspect& other) const;

		// get all common types
		scratch_vector<ComponentID> getIntersection(const Aspect& other) const;

		sizet size() const;

		bool empty() const;

		struct hash
		{
			sizet operator()(const Aspect& aspect) const;
		};

		~Aspect();
	};

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
		// Get count of registered aspects
		sizet size() const;

		// Get the registered aspect (returns nullptr if not found)
		const Aspect* getAspect(const Aspect& aspect) const;

		const Aspect* addOrGetAspect(const Aspect& aspect);

		// Remove an aspect from the registry
		bool removeAspect(const Aspect& aspect);

		// Get all descendants of an aspect 
		scratch_vector<const Aspect*> getDescendantAspects(const Aspect& aspect) const;
		// Get all ancestors of an aspect 
		scratch_vector<const Aspect*> getAncestorsAspects(const Aspect& aspect) const;
		// Find all aspects that intersect with the given aspect (have common components)
		scratch_vector<const Aspect*> findIntersectingAspects(const Aspect& aspect) const;

	private:
		// Add an aspect to the registry and return the node
		AspectNode* addOrGetAspectNode(const Aspect& aspect);

		// Get the node for an aspect (returns nullptr if not found)
		AspectNode* getNode(const Aspect& aspect) const;

		// Get all children of an aspect (aspects that contain this aspect)
		heap_vector<AspectNode*> getChildren(const Aspect& aspect) const;

		// Get parent of an aspect (most specific aspect that contains this one)
		AspectNode* getParent(const Aspect& aspect) const;


		// Get all descendants of an aspect (recursively all children)
		scratch_vector<AspectNode*> getDescendantsNodes(const Aspect& aspect) const;

		// Get all ancestors of an aspect (path to root)
		scratch_vector<AspectNode*> getAncestorsNodes(const Aspect& aspect) const;

		// Get the root node (empty aspect)
		AspectNode* getRoot() const;

		// Find all aspects that intersect with the given aspect (have common components)
		scratch_vector<AspectNode*> findIntersectingNodes(const Aspect& aspect) const;

		// Get all aspects in the registry (for iteration)
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

	template <typename Iterator>
	Aspect::Aspect(Iterator begin, Iterator end) requires std::is_base_of_v<
		std::forward_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>
	{
		sizet dist = eastl::distance(begin, end);
		if (dist <= 0) return;

		m_componentIds.reserve(dist);
		for (auto it = begin; it != end; ++it)
		{
			m_componentIds.emplace_back(*it);
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
}
