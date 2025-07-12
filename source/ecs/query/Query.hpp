#pragma once

#include "ecs/storage/Archetype.hpp"
#include "ecs/core/IComponent.hpp"

#include "base/CollectionUtilities.hpp"

namespace spite
{
	class Query
	{
	private:
		const ComponentMetadataRegistry* m_metadataRegistry;
		const Aspect* m_includeAspect;
		const Aspect* m_excludeAspect;
		const Aspect* m_mustBeEnabledAspect;
		const Aspect* m_mustBeModifiedAspect;
		heap_vector<Archetype*> m_archetypes;
		mutable bool m_wasModified = false;
		u64 m_includeVersion = 0;

		friend class QueryRegistry;

	public:
		Query(const ArchetypeManager& archetypeManager, const ComponentMetadataRegistry* registry,
		      const Aspect* includeAspect,
		      const Aspect* excludeAspect = nullptr,
		      const Aspect* mustBeEnabledAspect = nullptr,
		      const Aspect* mustBeModifiedAspect = nullptr) : m_metadataRegistry(registry),
		                                                      m_includeAspect(includeAspect),
		                                                      m_excludeAspect(excludeAspect),
		                                                      m_mustBeEnabledAspect(
			                                                      mustBeEnabledAspect),
		                                                      m_mustBeModifiedAspect(
			                                                      mustBeModifiedAspect)
		{
			SASSERTM(!m_includeAspect->intersects(*m_excludeAspect),
			         "Included aspect intersects with excluded aspect!\n")
			m_archetypes = archetypeManager.queryNonEmptyArchetypes(
				*m_includeAspect,
				*m_excludeAspect);
		}

		sizet getEntityCount();

		void rebuild(const ArchetypeManager& archetypeManager);

		bool wasModified() const;
		void resetModificationStatus() const;

		template <typename TFunc>
		void forEachChunk(TFunc& func)
		{
			for (Archetype* archetype : m_archetypes)
			{
				for (const auto& chunk : archetype->getChunks())
				{
					func(chunk.get());
				}
			}
		}

		template <bool IsConst = false, bool FilterEnabled = false, bool FilterModified = false, t_component...
		          TComponent
		>
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

			ScratchAllocator::ScopedMarker m_marker;

			scratch_vector<int> m_enabledIndicesInChunk;
			scratch_vector<int> m_modifiedIndicesInChunk;

			using single_component_type = std::tuple_element_t<0, std::tuple<TComponent...>>;

			void findNextValidEntity()
			{
				while (m_currentChunk)
				{
					while (m_entityIndexInChunk < m_currentChunk->count())
					{
						bool passedFilters = true;
						if constexpr (FilterEnabled)
						{
							for (const int componentIndex : m_enabledIndicesInChunk)
							{
								if (!m_currentChunk->isComponentEnabledByIndex(
									componentIndex,
									m_entityIndexInChunk))
								{
									passedFilters = false;
									break;
								}
							}
						}

						if (passedFilters)
						{
							if constexpr (FilterModified)
							{
								for (const int componentIndex : m_modifiedIndicesInChunk)
								{
									if (!m_currentChunk->wasModifiedLastFrameByIndex(
										componentIndex,
										m_entityIndexInChunk))
									{
										passedFilters = false;
										break;
									}
								}
							}
						}

						if (passedFilters)
						{
							return; // Found a valid entity
						}

						m_entityIndexInChunk++;
					}

					// Move to the next chunk
					++m_chunkIt;
					findNextValidChunk();
				}
			}

			void findNextValidChunk()
			{
				while (m_archetypeIt != m_query->m_archetypes.end())
				{
					if (*m_archetypeIt != m_currentArchetype)
					{
						m_currentArchetype = *m_archetypeIt;
						updateChunkCache();
						m_chunkIt = m_currentArchetype->getChunks().begin();
					}

					auto& chunks = m_currentArchetype->getChunks();
					while (m_chunkIt != chunks.end())
					{
						if (!(*m_chunkIt)->empty())
						{
							m_currentChunk = m_chunkIt->get();
							m_entityIndexInChunk = 0;
							return;
						}
						++m_chunkIt;
					}
					++m_archetypeIt;
				}
				m_currentChunk = nullptr; // End of iteration
			}


			void updateChunkCache()
			{
				const ComponentID types[] = {m_query->m_metadataRegistry->getComponentId(typeid(TComponent))...};
				for (size_t i = 0; i < sizeof...(TComponent); ++i)
				{
					m_componentIndicesInChunk[i] = m_currentArchetype->getComponentIndex(types[i]);
				}

				if constexpr (FilterEnabled)
				{
					if (m_query->m_mustBeEnabledAspect)
					{
						m_enabledIndicesInChunk.clear();
						const auto& enabledTypes = m_query->m_mustBeEnabledAspect->getComponentIds();
						if (!enabledTypes.empty())
						{
							m_enabledIndicesInChunk.reserve(enabledTypes.size());
							for (const auto& type : enabledTypes)
							{
								const int index = m_currentArchetype->getComponentIndex(type);
								if (index != -1)
								{
									m_enabledIndicesInChunk.push_back(index);
								}
							}
						}
					}
				}
				if constexpr (FilterModified)
				{
					if (m_query->m_mustBeModifiedAspect)
					{
						m_modifiedIndicesInChunk.clear();
						const auto& modifiedTypes = m_query->m_mustBeModifiedAspect->getComponentIds();
						if (!modifiedTypes.empty())
						{
							m_modifiedIndicesInChunk.reserve(modifiedTypes.size());
							for (const auto& type : modifiedTypes)
							{
								const int index = m_currentArchetype->getComponentIndex(type);
								if (index != -1)
								{
									m_modifiedIndicesInChunk.push_back(index);
								}
							}
						}
					}
				}
			}

			template <std::size_t... I>
			auto create_reference_tuple(
				std::index_sequence<I...>) -> eastl::tuple<std::conditional_t<
				IsConst, const TComponent&, TComponent&>...>
			{
				return eastl::forward_as_tuple(
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
			                                                  m_entityIndexInChunk(0),
			                                                  m_marker(
				                                                  FrameScratchAllocator::get().
				                                                  get_scoped_marker()),
			                                                  m_enabledIndicesInChunk(
				                                                  makeScratchVector<int>(
					                                                  FrameScratchAllocator::get())),
			                                                  m_modifiedIndicesInChunk(
				                                                  makeScratchVector<int>(
					                                                  FrameScratchAllocator::get()))
			{
				m_archetypeIt = m_query->m_archetypes.begin();
				if (isEnd || m_archetypeIt == m_query->m_archetypes.end())
				{
					m_archetypeIt = m_query->m_archetypes.end();
					m_currentChunk = nullptr;
				}
				else
				{
					findNextValidChunk();
					findNextValidEntity();
				}
			}

			Iterator& operator++()
			{
				m_entityIndexInChunk++;
				findNextValidEntity();
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
		Iterator<false, false, false, TComponent...> begin()
		{
			return Iterator<false, false, false, TComponent...>(this);
		}

		template <t_component... TComponent>
		Iterator<false, false, false, TComponent...> end()
		{
			return Iterator<false, false, false, TComponent...>(this, true);
		}

		template <t_component... TComponent>
		Iterator<true, false, false, TComponent...> cbegin() const
		{
			return Iterator<true, false, false, TComponent...>(this);
		}

		template <t_component... TComponent>
		Iterator<true, false, false, TComponent...> cend() const
		{
			return Iterator<true, false, false, TComponent...>(this, true);
		}

		template <t_component... TComponent>
		struct View
		{
			Query* m_query;

			View(Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->begin<TComponent...>(); }
			auto end() const { return m_query->end<TComponent...>(); }
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

		template <t_component... TComponent>
		using EnabledIterator = Iterator<false, true, false, TComponent...>;
		template <t_component... TComponent>
		using ConstEnabledIterator = Iterator<true, true, false, TComponent...>;

		template <t_component... TComponent>
		EnabledIterator<TComponent...> enabled_begin()
		{
			return EnabledIterator<TComponent...>(this);
		}

		template <t_component... TComponent>
		EnabledIterator<TComponent...> enabled_end()
		{
			return EnabledIterator<TComponent...>(this, true);
		}

		template <t_component... TComponent>
		ConstEnabledIterator<TComponent...> cenabled_begin() const
		{
			return ConstEnabledIterator<TComponent...>(this);
		}

		template <t_component... TComponent>
		ConstEnabledIterator<TComponent...> cenabled_end() const
		{
			return ConstEnabledIterator<TComponent...>(this, true);
		}

		template <t_component... TComponent>
		struct EnabledView
		{
			Query* m_query;

			EnabledView(Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->enabled_begin<TComponent...>(); }
			auto end() const { return m_query->enabled_end<TComponent...>(); }
		};

		template <t_component... TComponent>
		struct ConstEnabledView
		{
			const Query* m_query;

			ConstEnabledView(const Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->cenabled_begin<TComponent...>(); }
			auto end() const { return m_query->cenabled_end<TComponent...>(); }
		};

		template <t_component... TComponent>
		EnabledView<TComponent...> enabled_view() { return EnabledView<TComponent...>(this); }

		template <t_component... TComponent>
		ConstEnabledView<TComponent...> cenabled_view() const
		{
			return ConstEnabledView<TComponent...>(this);
		}

		template <t_component... TComponent>
		using ModifiedIterator = Iterator<false, false, true, TComponent...>;

		template <t_component... TComponent>
		using ConstModifiedIterator = Iterator<true, false, true, TComponent...>;

		template <t_component... TComponent>
		ModifiedIterator<TComponent...> modified_begin()
		{
			return ModifiedIterator<TComponent...>(this);
		}

		template <t_component... TComponent>
		ModifiedIterator<TComponent...> modified_end()
		{
			return ModifiedIterator<TComponent...>(this, true);
		}

		template <t_component... TComponent>
		ConstModifiedIterator<TComponent...> cmodified_begin() const
		{
			return ConstModifiedIterator<TComponent...>(this);
		}

		template <t_component... TComponent>
		ConstModifiedIterator<TComponent...> cmodified_end() const
		{
			return ConstModifiedIterator<TComponent...>(this, true);
		}

		template <t_component... TComponent>
		struct ModifiedView
		{
			Query* m_query;

			ModifiedView(Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->modified_begin<TComponent...>(); }
			auto end() const { return m_query->modified_end<TComponent...>(); }
		};

		template <t_component... TComponent>
		struct ConstModifiedView
		{
			const Query* m_query;

			ConstModifiedView(const Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->cmodified_begin<TComponent...>(); }
			auto end() const { return m_query->cmodified_end<TComponent...>(); }
		};

		template <t_component... TComponent>
		ModifiedView<TComponent...> modified_view() { return ModifiedView<TComponent...>(this); }

		template <t_component... TComponent>
		ConstModifiedView<TComponent...> cmodified_view() const
		{
			return ConstModifiedView<TComponent...>(this);
		}

		template <t_component... TComponent>
		using EnabledModifiedIterator = Iterator<false, true, true, TComponent...>;

		template <t_component... TComponent>
		using ConstEnabledModifiedIterator = Iterator<true, true, true, TComponent...>;

		template <t_component... TComponent>
		EnabledModifiedIterator<TComponent...> enabled_modified_begin()
		{
			return EnabledModifiedIterator<TComponent...>(this);
		}

		template <t_component... TComponent>
		EnabledModifiedIterator<TComponent...> enabled_modified_end()
		{
			return EnabledModifiedIterator<TComponent...>(this, true);
		}

		template <t_component... TComponent>
		ConstEnabledModifiedIterator<TComponent...> cenabled_modified_begin() const
		{
			return ConstEnabledModifiedIterator<TComponent...>(this);
		}

		template <t_component... TComponent>
		ConstEnabledModifiedIterator<TComponent...> cenabled_modified_end() const
		{
			return ConstEnabledModifiedIterator<TComponent...>(this, true);
		}

		template <t_component... TComponent>
		struct EnabledModifiedView
		{
			Query* m_query;

			EnabledModifiedView(Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->enabled_modified_begin<TComponent...>(); }
			auto end() const { return m_query->enabled_modified_end<TComponent...>(); }
		};

		template <t_component... TComponent>
		struct ConstEnabledModifiedView
		{
			const Query* m_query;

			ConstEnabledModifiedView(const Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->cenabled_modified_begin<TComponent...>(); }
			auto end() const { return m_query->cenabled_modified_end<TComponent...>(); }
		};

		template <t_component... TComponent>
		EnabledModifiedView<TComponent...> enabled_modified_view()
		{
			return EnabledModifiedView<TComponent...>(this);
		}

		template <t_component... TComponent>
		ConstEnabledModifiedView<TComponent...> cenabled_modified_view() const
		{
			return ConstEnabledModifiedView<TComponent...>(this);
		}
	};
}
