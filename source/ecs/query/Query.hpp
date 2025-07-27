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
		const Aspect* m_readAspect;
		const Aspect* m_writeAspect;
		const Aspect* m_excludeAspect;
		const Aspect* m_mustBeEnabledAspect;
		const Aspect* m_mustBeModifiedAspect;
		heap_vector<Archetype*> m_archetypes;
		u64 m_includeVersion = 0;

		friend class QueryRegistry;

	public:
		Query(const ArchetypeManager* archetypeManager,const Aspect* includeAspect, const Aspect* readAspect, const Aspect* writeAspect,
		      const Aspect* excludeAspect = nullptr,
		      const Aspect* mustBeEnabledAspect = nullptr,
		      const Aspect* mustBeModifiedAspect = nullptr);

		sizet getEntityCount();

		void rebuild(const ArchetypeManager& archetypeManager);

		template <typename TFunc>
		void forEachChunk(TFunc& func)
		{
			for (Archetype* archetype : m_archetypes)
			{
				for (const auto& chunk : archetype->getChunks())
				{
					func(chunk);
				}
			}
		}

		template <typename... TArgs>
		class Iterator
		{
		private:
			Query* m_query;

			heap_vector<Archetype*>::const_iterator m_archetypeIt;
			heap_vector<Chunk*>::const_iterator m_chunkIt;
			sizet m_entityIndexInChunk;

			Chunk* m_currentChunk = nullptr;
			Archetype* m_currentArchetype = nullptr;

			static_assert(((is_read_wrapper_v<TArgs> || is_write_wrapper_v<TArgs> || is_entity_v<TArgs>) && ...),
			              "All arguments to a query view must be Read<T>, Write<T>, or Entity.");

			static constexpr sizet entity_count = (is_entity_v<TArgs> + ...);
			static_assert(entity_count <= 1, "Cannot request Entity more than once in a query view.");

			static constexpr sizet component_count = ((is_read_wrapper_v<TArgs> || is_write_wrapper_v<TArgs>) + ...);

			// Maps the index in TArgs... to the index in the component-only list. -1 if not a component.
			static constexpr eastl::array<int, sizeof...(TArgs)> arg_to_comp_idx_map = []
			{
				eastl::array<int, sizeof...(TArgs)> map{};
				int current_comp_idx = 0;
				int current_arg_idx = 0;
				auto set_map = [&]<typename T0>(std::type_identity<T0>)
				{
					if constexpr (is_read_wrapper_v<T0> || is_write_wrapper_v<T0>)
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
						if (!m_enabledIndicesInChunk.empty())
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

						if (passedFilters && !m_modifiedIndicesInChunk.empty())
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
							m_currentChunk = *m_chunkIt;
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
					auto get_component_indices = [&]<typename T0>(std::type_identity<T0>)
					{
						if constexpr (is_read_wrapper_v<T0> || is_write_wrapper_v<T0>)
						{
							using ComponentType = get_component_type<T0>;
							m_componentIndicesInChunk[current_comp_idx++] = m_currentArchetype->getComponentIndex(
								ComponentMetadataRegistry::getComponentId<ComponentType>());
						}
					};
					(get_component_indices(std::type_identity<TArgs>{}), ...);
				}

				m_enabledIndicesInChunk.clear();
				if (m_query->m_mustBeEnabledAspect && !m_query->m_mustBeEnabledAspect->empty())
				{
					const auto& enabledTypes = m_query->m_mustBeEnabledAspect->getComponentIds();
					m_enabledIndicesInChunk.reserve(enabledTypes.size());
					for (const auto& type : enabledTypes)
					{
						const int index = m_currentArchetype->getComponentIndex(type);
						if (index != -1) m_enabledIndicesInChunk.push_back(index);
					}
				}

				m_modifiedIndicesInChunk.clear();
				if (m_query->m_mustBeModifiedAspect && !m_query->m_mustBeModifiedAspect->empty())
				{
					const auto& modifiedTypes = m_query->m_mustBeModifiedAspect->getComponentIds();
					m_modifiedIndicesInChunk.reserve(modifiedTypes.size());
					for (const auto& type : modifiedTypes)
					{
						const int index = m_currentArchetype->getComponentIndex(type);
						if (index != -1) m_modifiedIndicesInChunk.push_back(index);
					}
				}
			}

			template <typename T, typename = void>
			struct reference_type_for_helper;

			template <typename T>
			struct reference_type_for_helper<T, std::enable_if_t<is_read_wrapper_v<T> || is_write_wrapper_v<T>>>
			{
				using type = std::conditional_t<is_write_wrapper_v<T>, get_component_type<T>&, const get_component_type<T>&>;
			};

			template <typename T>
			struct reference_type_for_helper<T, std::enable_if_t<is_entity_v<T>>>
			{
				using type = Entity;
			};

			template <typename T>
			using reference_type_for = typename reference_type_for_helper<T>::type;

			template <typename T, typename = void>
			struct pointer_type_for_helper;

			template <typename T>
			struct pointer_type_for_helper<T, std::enable_if_t<is_read_wrapper_v<T> || is_write_wrapper_v<T>>>
			{
				using type = std::conditional_t<is_write_wrapper_v<T>, get_component_type<T>*, const get_component_type<T>*>;
			};

			template <typename T>
			struct pointer_type_for_helper<T, std::enable_if_t<is_entity_v<T>>>
			{
				using type = Entity*;
			};

			template <typename T>
			using pointer_type_for = typename pointer_type_for_helper<T>::type;

			template <typename ArgType, sizet ArgIdx>
			auto get_arg() -> reference_type_for<ArgType>
			{
				if constexpr (is_entity_v<ArgType>)
				{
					return getEntity();
				}
				else // is Read<T> or Write<T>
				{
					using ComponentType = get_component_type<ArgType>;
					ComponentID componentId = ComponentMetadataRegistry::getComponentId<ComponentType>();
					constexpr int component_array_idx = arg_to_comp_idx_map[ArgIdx];
					static_assert(component_array_idx != -1);
					const int component_idx_in_chunk = m_componentIndicesInChunk[component_array_idx];

					if constexpr (is_write_wrapper_v<ArgType>)
					{
						SASSERTM(m_query->m_writeAspect->contains(componentId),
						        "Write<T> requested for a component not declared with with_write()!")
						m_currentChunk->markModifiedByIndex(component_idx_in_chunk, m_entityIndexInChunk);
						return *static_cast<ComponentType*>(m_currentChunk->getComponentDataPtrByIndex(
							component_idx_in_chunk,
							m_entityIndexInChunk));
					}
					else // is_read_wrapper_v<ArgType>
					{
						SASSERTM(m_query->m_readAspect->contains(componentId) || m_query->m_writeAspect->contains(
							        componentId), "Read<T> requested for a component with no declared dependency!")
						return *static_cast<const ComponentType*>(m_currentChunk->getComponentDataPtrByIndex(
							component_idx_in_chunk,
							m_entityIndexInChunk));
					}
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

			Iterator(Query* query, bool isEnd = false) : m_query(query),
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
		Iterator<TArgs...> begin()
		{
			return Iterator<TArgs...>(this);
		}

		template <typename... TArgs>
		Iterator<TArgs...> end()
		{
			return Iterator<TArgs...>(this, true);
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
		View<TArgs...> view() { return View<TArgs...>(this); }
	};
}