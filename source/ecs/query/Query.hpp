#pragma once

#include "ecs/storage/Archetype.hpp"
#include "ecs/core/IComponent.hpp"

#include "base/CollectionUtilities.hpp"

#include "ecs/storage/ArchetypeManager.hpp"

namespace spite
{
	class Query
	{
	private:
		const Aspect* m_includeAspect;
		const Aspect* m_excludeAspect;
		const Aspect* m_mustBeEnabledAspect;
		const Aspect* m_mustBeModifiedAspect;
		heap_vector<Archetype*> m_archetypes;
		mutable bool m_wasModified = false;
		u64 m_includeVersion = 0;

		friend class QueryRegistry;

	public:
		Query(const ArchetypeManager* archetypeManager, const Aspect* includeAspect,
		      const Aspect* excludeAspect = nullptr,
		      const Aspect* mustBeEnabledAspect = nullptr,
		      const Aspect* mustBeModifiedAspect = nullptr);

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

		template <bool IsConst = false, bool FilterEnabled = false, bool FilterModified = false, typename... TArgs>
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

			// --- Metaprogramming helpers ---
			template <typename T>
			static constexpr bool is_entity_v = std::is_same_v<std::decay_t<T>, Entity>;

			template <typename T>
			static constexpr bool is_component_v = t_component<T>;

			static constexpr sizet entity_count = (is_entity_v<TArgs> + ...);
			static_assert(entity_count <= 1, "Cannot request Entity more than once in a query view.");

			static constexpr sizet component_count = (is_component_v<TArgs> + ...);
			static_assert(component_count + entity_count == sizeof...(TArgs),
			              "All arguments to a query view must be either a component type or Entity.");

			// Maps the index in TArgs... to the index in the component-only list. -1 if not a component.
			static constexpr eastl::array<int, sizeof...(TArgs)> arg_to_comp_idx_map = []
			{
				eastl::array<int, sizeof...(TArgs)> map{};
				int current_comp_idx = 0;
				int current_arg_idx = 0;
				auto set_map = [&]<typename T0>(T0)
				{
					using T = typename T0::type;
					if constexpr (is_component_v<T>)
					{
						map[current_arg_idx] = current_comp_idx++;
					}
					else
					{
						map[current_arg_idx] = -1;
					}
					current_arg_idx++;
				};
				(set_map(std::type_identity<TArgs>{}), ...);
				return map;
			}();

			eastl::array<int, component_count> m_componentIndicesInChunk;

			ScratchAllocator::ScopedMarker m_marker;
			scratch_vector<int> m_enabledIndicesInChunk;
			scratch_vector<int> m_modifiedIndicesInChunk;

			using first_arg_type = std::tuple_element_t<0, std::tuple<TArgs...>>;

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
								if (!m_currentChunk->isComponentEnabledByIndex(componentIndex, m_entityIndexInChunk))
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
									if (!m_currentChunk->wasModifiedLastFrameByIndex(componentIndex,
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
				if constexpr (component_count > 0)
				{
					int current_comp_idx = 0;
					auto get_component_indices = [&]<typename T0>(T0)
					{
						using T = typename T0::type;
						if constexpr (is_component_v<T>)
						{
							m_componentIndicesInChunk[current_comp_idx++] = m_currentArchetype->getComponentIndex(
								ComponentMetadataRegistry::getComponentId<T>());
						}
					};
					(get_component_indices(std::type_identity<TArgs>{}), ...);
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

			template <typename T>
			using reference_type_for = std::conditional_t<
				is_entity_v<T>,
				Entity, // Return Entity by value
				std::conditional_t<IsConst, const T&, T&>
			>;

			template <typename T>
			using pointer_type_for = std::conditional_t<
				is_entity_v<T>,
				Entity*, // Pointer to entity
				std::conditional_t<IsConst, const T*, T*>
			>;

			template <typename ArgType, sizet ArgIdx>
			reference_type_for<ArgType> get_arg()
			{
				if constexpr (is_entity_v<ArgType>)
				{
					return getEntity();
				}
				else // is_component_v
				{
					constexpr int component_array_idx = arg_to_comp_idx_map[ArgIdx];
					static_assert(component_array_idx != -1);
					const int component_idx_in_chunk = m_componentIndicesInChunk[component_array_idx];

					if constexpr (!IsConst)
					{
						m_currentChunk->markModifiedByIndex(component_idx_in_chunk, m_entityIndexInChunk);
					}

					using ComponentType = std::conditional_t<IsConst, const ArgType, ArgType>;
					return *static_cast<ComponentType*>(m_currentChunk->getComponentDataPtrByIndex(
						component_idx_in_chunk,
						m_entityIndexInChunk));
				}
			}

			template <sizet... I>
			auto create_reference_tuple(std::index_sequence<I...>) -> eastl::tuple<reference_type_for<TArgs>...>
			{
				return eastl::forward_as_tuple(
					get_arg<std::tuple_element_t<I, std::tuple<TArgs...>>, I>()...);
			}

		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;

			using reference = std::conditional_t<
				sizeof...(TArgs) == 1,
				reference_type_for<first_arg_type>,
				eastl::tuple<reference_type_for<TArgs>...>
			>;
			using value_type = std::conditional_t<
				sizeof...(TArgs) == 1,
				std::decay_t<reference_type_for<first_arg_type>>,
				eastl::tuple<std::decay_t<reference_type_for<TArgs>>...>
			>;
			using pointer = std::conditional_t<
				sizeof...(TArgs) == 1,
				pointer_type_for<first_arg_type>,
				void // Returning a pointer to a temporary tuple is not feasible
			>;

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

				if constexpr (sizeof...(TArgs) == 1)
				{
					return get_arg<first_arg_type, 0>();
				}
				else
				{
					return create_reference_tuple(std::index_sequence_for<TArgs...>{});
				}
			}

			Entity getEntity() const
			{
				return m_currentChunk->entity(m_entityIndexInChunk);
			}

			bool operator==(const Iterator& other) const
			{
				if (m_currentChunk == nullptr && other.m_currentChunk == nullptr) return true;
				if (m_currentChunk == nullptr || other.m_currentChunk == nullptr) return false;
				return m_query == other.m_query && m_archetypeIt == other.m_archetypeIt && m_chunkIt
					== other.m_chunkIt && m_entityIndexInChunk == other.m_entityIndexInChunk;
			}

			bool operator!=(const Iterator& other) const
			{
				return !(*this == other);
			}
		};

		template <typename... TArgs>
		Iterator<false, false, false, TArgs...> begin()
		{
			return Iterator<false, false, false, TArgs...>(this);
		}

		template <typename... TArgs>
		Iterator<false, false, false, TArgs...> end()
		{
			return Iterator<false, false, false, TArgs...>(this, true);
		}

		template <typename... TArgs>
		Iterator<true, false, false, TArgs...> cbegin() const
		{
			return Iterator<true, false, false, TArgs...>(this);
		}

		template <typename... TArgs>
		Iterator<true, false, false, TArgs...> cend() const
		{
			return Iterator<true, false, false, TArgs...>(this, true);
		}

		template <typename... TArgs>
		struct View
		{
			Query* m_query;

			View(Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->begin<TArgs...>(); }
			auto end() const { return m_query->end<TArgs...>(); }
		};

		template <typename... TArgs>
		struct ConstView
		{
			const Query* m_query;

			ConstView(const Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->cbegin<TArgs...>(); }
			auto end() const { return m_query->cend<TArgs...>(); }
		};

		template <typename... TArgs>
		View<TArgs...> view() { return View<TArgs...>(this); }

		template <typename... TArgs>
		ConstView<TArgs...> view() const { return ConstView<TArgs...>(this); }

		template <typename... TArgs>
		ConstView<TArgs...> cview() const { return ConstView<TArgs...>(this); }

		template <typename... TArgs>
		using EnabledIterator = Iterator<false, true, false, TArgs...>;
		template <typename... TArgs>
		using ConstEnabledIterator = Iterator<true, true, false, TArgs...>;

		template <typename... TArgs>
		EnabledIterator<TArgs...> enabled_begin()
		{
			return EnabledIterator<TArgs...>(this);
		}

		template <typename... TArgs>
		EnabledIterator<TArgs...> enabled_end()
		{
			return EnabledIterator<TArgs...>(this, true);
		}

		template <typename... TArgs>
		ConstEnabledIterator<TArgs...> cenabled_begin() const
		{
			return ConstEnabledIterator<TArgs...>(this);
		}

		template <typename... TArgs>
		ConstEnabledIterator<TArgs...> cenabled_end() const
		{
			return ConstEnabledIterator<TArgs...>(this, true);
		}

		template <typename... TArgs>
		struct EnabledView
		{
			Query* m_query;

			EnabledView(Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->enabled_begin<TArgs...>(); }
			auto end() const { return m_query->enabled_end<TArgs...>(); }
		};

		template <typename... TArgs>
		struct ConstEnabledView
		{
			const Query* m_query;

			ConstEnabledView(const Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->cenabled_begin<TArgs...>(); }
			auto end() const { return m_query->cenabled_end<TArgs...>(); }
		};

		template <typename... TArgs>
		EnabledView<TArgs...> enabled_view() { return EnabledView<TArgs...>(this); }

		template <typename... TArgs>
		ConstEnabledView<TArgs...> cenabled_view() const
		{
			return ConstEnabledView<TArgs...>(this);
		}



		template <typename... TArgs>
		using ModifiedIterator = Iterator<false, false, true, TArgs...>;

		template <typename... TArgs>
		using ConstModifiedIterator = Iterator<true, false, true, TArgs...>;

		template <typename... TArgs>
		ModifiedIterator<TArgs...> modified_begin()
		{
			return ModifiedIterator<TArgs...>(this);
		}

		template <typename... TArgs>
		ModifiedIterator<TArgs...> modified_end()
		{
			return ModifiedIterator<TArgs...>(this, true);
		}

		template <typename... TArgs>
		ConstModifiedIterator<TArgs...> cmodified_begin() const
		{
			return ConstModifiedIterator<TArgs...>(this);
		}

		template <typename... TArgs>
		ConstModifiedIterator<TArgs...> cmodified_end() const
		{
			return ConstModifiedIterator<TArgs...>(this, true);
		}

		template <typename... TArgs>
		struct ModifiedView
		{
			Query* m_query;

			ModifiedView(Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->modified_begin<TArgs...>(); }
			auto end() const { return m_query->modified_end<TArgs...>(); }
		};

		template <typename... TArgs>
		struct ConstModifiedView
		{
			const Query* m_query;

			ConstModifiedView(const Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->cmodified_begin<TArgs...>(); }
			auto end() const { return m_query->cmodified_end<TArgs...>(); }
		};

		template <typename... TArgs>
		ModifiedView<TArgs...> modified_view() { return ModifiedView<TArgs...>(this); }

		template <typename... TArgs>
		ConstModifiedView<TArgs...> cmodified_view() const
		{
			return ConstModifiedView<TArgs...>(this);
		}

		template <typename... TArgs>
		using EnabledModifiedIterator = Iterator<false, true, true, TArgs...>;

		template <typename... TArgs>
		using ConstEnabledModifiedIterator = Iterator<true, true, true, TArgs...>;

		template <typename... TArgs>
		EnabledModifiedIterator<TArgs...> enabled_modified_begin()
		{
			return EnabledModifiedIterator<TArgs...>(this);
		}

		template <typename... TArgs>
		EnabledModifiedIterator<TArgs...> enabled_modified_end()
		{
			return EnabledModifiedIterator<TArgs...>(this, true);
		}

		template <typename... TArgs>
		ConstEnabledModifiedIterator<TArgs...> cenabled_modified_begin() const
		{
			return ConstEnabledModifiedIterator<TArgs...>(this);
		}

		template <typename... TArgs>
		ConstEnabledModifiedIterator<TArgs...> cenabled_modified_end() const
		{
			return ConstEnabledModifiedIterator<TArgs...>(this, true);
		}

		template <typename... TArgs>
		struct EnabledModifiedView
		{
			Query* m_query;

			EnabledModifiedView(Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->enabled_modified_begin<TArgs...>(); }
			auto end() const { return m_query->enabled_modified_end<TArgs...>(); }
		};

		template <typename... TArgs>
		struct ConstEnabledModifiedView
		{
			const Query* m_query;

			ConstEnabledModifiedView(const Query* query) : m_query(query)
			{
			}

			auto begin() const { return m_query->cenabled_modified_begin<TArgs...>(); }
			auto end() const { return m_query->cenabled_modified_end<TArgs...>(); }
		};

		template <typename... TArgs>
		EnabledModifiedView<TArgs...> enabled_modified_view()
		{
			return EnabledModifiedView<TArgs...>(this);
		}

		template <typename... TArgs>
		ConstEnabledModifiedView<TArgs...> cenabled_modified_view() const
		{
			return ConstEnabledModifiedView<TArgs...>(this);
		}
	};
}
