#pragma once

#include "Archetype.hpp"
#include "IComponent.hpp"

namespace spite
{
	class Query
	{
	private:
		Aspect m_includeAspect;
		Aspect m_excludeAspect;
		heap_vector<Archetype*> m_archetypes;
		mutable bool m_wasModified = false;

	public:
		Query(const ArchetypeManager& archetypeManager,
		      Aspect includeAspect,
		      Aspect excludeAspect = Aspect());

		void rebuild(const ArchetypeManager& archetypeManager);

		bool wasModified() const;
		void resetModificationStatus() const;

		template <typename TFunc>
		void forEachChunk(TFunc&& func)
		{
			for (Archetype* archetype : m_archetypes)
			{
				for (const auto& chunk : archetype->getChunks())
				{
					func(chunk.get());
				}
			}
		}

		template <t_component... TComponent, bool IsConst = false>
		class Iterator
		{
		private:
			using query_ptr_t = std::conditional_t<IsConst, const Query*, Query*>;
			query_ptr_t m_query;

			heap_vector<Archetype*>::const_iterator m_archetypeIt;
			heap_vector<std::unique_ptr<Chunk>>::const_iterator m_chunkIt;
			sizet m_entityIndexInChunk;

			using ChunkPtr = std::conditional_t<IsConst, const Chunk*, Chunk*>;
			ChunkPtr m_currentChunk = nullptr;
			Archetype* m_currentArchetype = nullptr;
			eastl::array<int, sizeof...(TComponent)> m_componentIndicesInChunk;

			using single_component_type = std::tuple_element_t<0, std::tuple<TComponent...>>;

			void findNextValidChunk()
			{
				while (m_archetypeIt != m_query->m_archetypes.end())
				{
					//transition to a new archetype
					if (*m_archetypeIt != m_currentArchetype)
					{
						m_currentArchetype = *m_archetypeIt;
						updateChunkCache();
					}

					auto& chunks = m_currentArchetype->getChunks();
					while (m_chunkIt != chunks.end())
					{
						//transition to a new chunk
						if (!(*m_chunkIt)->empty())
						{
							m_currentChunk = m_chunkIt->get();
							m_entityIndexInChunk = 0;
							return;
						}
						++m_chunkIt;
					}
					++m_archetypeIt;
					if (m_archetypeIt != m_query->m_archetypes.end())
					{
						m_chunkIt = (*m_archetypeIt)->getChunks().begin();
					}
				}
				m_currentChunk = nullptr; // End of iteration
			}

			void updateChunkCache()
			{
				const std::type_index types[] = {typeid(TComponent)...};
				for (size_t i = 0; i < sizeof...(TComponent); ++i)
				{
					m_componentIndicesInChunk[i] = m_currentArchetype->getComponentIndex(types[i]);
				}
			}

			template <std::size_t... I>
			auto create_reference_tuple(
				std::index_sequence<I...>) -> eastl::tuple<std::conditional_t<
				IsConst, const TComponent&, TComponent&>...>
			{
				return eastl::make_tuple(
					*static_cast<std::conditional_t<IsConst, const TComponent*, TComponent*>>(
						m_currentChunk->getComponentDataPtrByIndex(
							m_componentIndicesInChunk[I],
							m_entityIndexInChunk))...);
			}

		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::conditional_t<
				sizeof...(TComponent) == 1, single_component_type, eastl::tuple<TComponent...>>;
			using pointer = std::conditional_t<
				sizeof...(TComponent) == 1, std::conditional_t<
					IsConst, const single_component_type*, single_component_type*>, eastl::tuple<
					std::conditional_t<IsConst, const TComponent*, TComponent*>...>>;
			using reference = std::conditional_t<
				sizeof...(TComponent) == 1, std::conditional_t<
					IsConst, const single_component_type&, single_component_type&>, eastl::tuple<
					std::conditional_t<IsConst, const TComponent&, TComponent&>...>>;

			Iterator(query_ptr_t query, bool isEnd = false) : m_query(query),
			                                                  m_entityIndexInChunk(0)
			{
				m_archetypeIt = m_query->m_archetypes.begin();
				if (isEnd)
				{
					m_archetypeIt = m_query->m_archetypes.end();
					m_currentChunk = nullptr;
				}
				else
				{
					if (m_archetypeIt != m_query->m_archetypes.end())
					{
						m_chunkIt = (*m_archetypeIt)->getChunks().begin();
						findNextValidChunk();
					}
					else
					{
						m_currentChunk = nullptr;
					}
				}
			}

			Iterator& operator++()
			{
				m_entityIndexInChunk++;
				if (m_currentChunk && m_entityIndexInChunk >= m_currentChunk->count())
				{
					++m_chunkIt;
					findNextValidChunk();
				}
				return *this;
			}

			Iterator operator++(int)
			{
				Iterator tmp = *this;
				++(*this);
				return tmp;
			}

			reference operator*()
			{
				if constexpr (!IsConst)
				{
					m_query->m_wasModified = true;
				}

				if constexpr (sizeof...(TComponent) == 1)
				{
					const int componentIndex = m_componentIndicesInChunk[0];
					if constexpr (!IsConst)
					{
						m_currentChunk->markModifiedByIndex(componentIndex, m_entityIndexInChunk);
					}
					using ComponentType = std::conditional_t<
						IsConst, const single_component_type, single_component_type>;
					return *static_cast<ComponentType*>(m_currentChunk->getComponentDataPtrByIndex(
						componentIndex,
						m_entityIndexInChunk));
				}
				else
				{
					if constexpr (!IsConst)
					{
						for (size_t i = 0; i < sizeof...(TComponent); ++i)
						{
							m_currentChunk->markModifiedByIndex(
								m_componentIndicesInChunk[i],
								m_entityIndexInChunk);
						}
					}
					return create_reference_tuple(std::index_sequence_for<TComponent...>{});
				}
			}

			Entity getEntity() const
			{
				return m_currentChunk->entity(m_entityIndexInChunk);
			}

			bool operator==(const Iterator& other) const
			{
				if (m_currentChunk == nullptr && other.m_currentChunk == nullptr) return true;
				return m_query == other.m_query && m_archetypeIt == other.m_archetypeIt && m_chunkIt
					== other.m_chunkIt && m_entityIndexInChunk == other.m_entityIndexInChunk;
			}

			bool operator!=(const Iterator& other) const
			{
				return !(*this == other);
			}
		};

		template <t_component... TComponent>
		Iterator<TComponent...> begin() { return Iterator<TComponent...>(this); }

		template <t_component... TComponent>
		Iterator<TComponent...> end() { return Iterator<TComponent...>(this, true); }

		template <t_component... TComponent>
		Iterator<TComponent..., true> cbegin() const { return Iterator<TComponent..., true>(this); }

		template <t_component... TComponent>
		Iterator<TComponent..., true> cend() const
		{
			return Iterator<TComponent..., true>(this, true);
		}

		template <t_component... TComponent>
		struct View
		{
			Query* m_query;

			View(Query* query) : m_query(query)
			{
			}

			auto begin() { return m_query->begin<TComponent...>(); }
			auto end() { return m_query->end<TComponent...>(); }
		};

		template <t_component... TComponent>
		struct ConstView
		{
			const Query* m_query;

			ConstView(const Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->cbegin<TComponent...>(); }
			auto end() const { return m_query->cend<TComponent...>(); }
		};

		template <t_component... TComponent>
		View<TComponent...> view() { return View<TComponent...>(this); }

		template <t_component... TComponent>
		ConstView<TComponent...> view() const { return ConstView<TComponent...>(this); }

		template <t_component... TComponent>
		ConstView<TComponent...> cview() const { return ConstView<TComponent...>(this); }
	};
}
