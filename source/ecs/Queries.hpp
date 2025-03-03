#pragma once

#include "base/LoggingTestStrings.hpp"
#include <EASTL/tuple.h>
#include "ecs/Core.hpp"

namespace spite
{
	class IQuery
	{
		using TypesVector = eastl::vector<std::type_index, spite::HeapAllocator>;

	public:
		virtual void recreate(ComponentLookup* lookup, ComponentStorage* storage, const TypesVector& hasComponents,
		                      const TypesVector& hasNoComponents) = 0;
		virtual ~IQuery() = default;
	};


	template <t_plain_component T>
	class Query1 final : public IQuery
	{
		using IndicesVector = eastl::vector<sizet, spite::HeapAllocator>;
		IndicesVector m_indices;

		ComponentTable<T>* m_table;

		using TypesVector = eastl::vector<std::type_index, spite::HeapAllocator>;

	public:
		Query1(ComponentLookup* lookup, ComponentStorage* storage,
		       const spite::HeapAllocator& allocator, const TypesVector& hasComponents,
		       const TypesVector& hasNoComponents) : m_indices(allocator)
		{
			//if the table is not registered upon query creation
			m_table = &storage->getComponentsSafe<T>();
			recreate(lookup, storage, hasComponents, hasNoComponents);
		}

		Query1(const Query1& other) = delete;
		Query1(Query1&& other) = delete;
		Query1& operator=(const Query1& other) = delete;
		Query1& operator=(Query1&& other) = delete;

		sizet getSize() const
		{
			return m_indices.size();
		}

		sizet getComponentIndex(const sizet filterIndex)
		{
			return m_indices[filterIndex];
		}

		T& operator[](const sizet n)
		{
			return m_table->operator[](m_indices[n]);
		}

		bool& componentState(const sizet n)
		{
			return m_table->isActive(m_indices[n]);
		}

		Entity componentOwner(const sizet n)
		{
			return m_table->owner(m_indices[n]);
		}
		
		void recreate(ComponentLookup* lookup, ComponentStorage* storage, const TypesVector& hasComponents,
		              const TypesVector& hasNoComponents) override
		{
			m_indices.clear();
			m_table = &storage->getComponentsAsserted<T>();

			for (sizet i = 0, size = m_table->getOccupiedSize(); i < size; ++i)
			{
				Entity entity = m_table->owner(i);

				bool matchesCondition = true;
				for (const auto& componentType : hasComponents)
				{
					matchesCondition &= lookup->hasComponent(entity, componentType);
				}
				for (const auto& componentType : hasNoComponents)
				{
					matchesCondition &= !lookup->hasComponent(entity, componentType);
				}

				if (matchesCondition)
				{
					m_indices.push_back(i);
				}
			}
		}

		class iterator
		{
			ComponentTable<T>* m_table;

			IndicesVector::iterator m_current;
			IndicesVector::iterator m_end;

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			iterator(const IndicesVector::iterator current, const IndicesVector::iterator end, ComponentTable<T>* table)
				: m_table(table), m_current(current), m_end(end)
			{
			}

			iterator& operator++()
			{
				++m_current;
				return *this;
			}

			reference operator*() const
			{
				return m_table->operator[](*m_current);
			}

			bool operator!=(const iterator& other) const
			{
				return m_current != other.m_current;
			}
		};

		class exclude_inactive_iterator
		{
			ComponentTable<T>* m_table;
			IndicesVector::iterator m_current;
			IndicesVector::iterator m_end;

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			exclude_inactive_iterator(const IndicesVector::iterator current,
			                          const IndicesVector::iterator end,
			                          ComponentTable<T>* table)
				: m_table(table), m_current(current), m_end(end)
			{
				// Skip inactive elements 
				while (m_current != m_end && !m_table->isActive(*m_current))
				{
					++m_current;
				}
			}

			exclude_inactive_iterator& operator++()
			{
				++m_current;

				// Skip inactive elements
				while (m_current != m_end && !m_table->isActive(*m_current))
				{
					++m_current;
				}
				return *this;
			}

			exclude_inactive_iterator operator++(int)
			{
				exclude_inactive_iterator tmp = *this;
				++(*this);
				return tmp;
			}

			reference operator*() const
			{
				return m_table->operator[](*m_current);
			}

			pointer operator->() const
			{
				return &(m_table->operator[](*m_current));
			}

			bool operator==(const exclude_inactive_iterator& other) const
			{
				return m_current == other.m_current;
			}

			bool operator!=(const exclude_inactive_iterator& other) const
			{
				return !(*this == other);
			}
		};

		iterator begin()
		{
			return iterator(m_indices.begin(), m_indices.end(), m_table);
		}

		iterator end()
		{
			return iterator(m_indices.end(), m_indices.end(), m_table);
		}

		exclude_inactive_iterator exclude_inactive_begin()
		{
			return exclude_inactive_iterator(m_indices.begin(), m_indices.end(), m_table);
		}

		exclude_inactive_iterator exclude_inactive_end()
		{
			return exclude_inactive_iterator(m_indices.end(), m_indices.end(), m_table);
		}

		struct ExcludeInactiveView
		{
			Query1& query;

			auto begin() { return query.exclude_inactive_begin(); }
			auto end() { return query.exclude_inactive_end(); }
		};

		ExcludeInactiveView excludeInactive()
		{
			return ExcludeInactiveView{*this};
		}

		~Query1() override = default;
	};

	template <t_plain_component T1, t_plain_component T2>
	class Query2 final : public IQuery
	{
		using IndicesVector = eastl::vector<eastl::tuple<sizet, sizet>, spite::HeapAllocator>;
		IndicesVector m_indices;

		ComponentTable<T1>* m_table1;
		ComponentTable<T2>* m_table2;

		using TypesVector = eastl::vector<std::type_index, spite::HeapAllocator>;

	public:
		Query2(ComponentLookup* lookup, ComponentStorage* storage,
		       const spite::HeapAllocator& allocator, const TypesVector& hasComponents,
		       const TypesVector& hasNoComponents) : m_indices(allocator)
		{
			//if the table is not registered upon query creation
			m_table1 = &storage->getComponentsSafe<T1>();
			m_table2 = &storage->getComponentsSafe<T2>();
			recreate(lookup, storage, hasComponents, hasNoComponents);
		}

		Query2(const Query2& other) = delete;
		Query2(Query2&& other) = delete;
		Query2& operator=(const Query2& other) = delete;
		Query2& operator=(Query2&& other) = delete;

		sizet getSize() const
		{
			return m_indices.size();
		}

		sizet getComponentIndexT1(const sizet filterIndex)
		{
			return eastl::get<0>(m_indices[filterIndex]);
		}

		sizet getComponentIndexT2(const sizet filterIndex)
		{
			return eastl::get<1>(m_indices[filterIndex]);
		}

		T1& getComponentT1(const sizet n)
		{
			return m_table1->operator[](eastl::get<0>(m_indices[n]));
		}

		T2& getComponentT2(const sizet n)
		{
			return m_table2->operator[](eastl::get<1>(m_indices[n]));
		}

		void recreate(ComponentLookup* lookup, ComponentStorage* storage, const TypesVector& hasComponents,
		              const TypesVector& hasNoComponents) override
		{
			m_table1 = &storage->getComponentsAsserted<T1>();
			m_table2 = &storage->getComponentsAsserted<T2>();

			std::type_index t2Type = std::type_index(typeid(T2));

			for (sizet i = 0, size = m_table1->getOccupiedSize(); i < size; ++i)
			{
				Entity entity = m_table1->operator[](i).owner;

				bool matchesCondition = true;
				for (const auto& componentType : hasComponents)
				{
					matchesCondition &= lookup->hasComponent(entity, componentType);
				}
				for (const auto& componentType : hasNoComponents)
				{
					matchesCondition &= !lookup->hasComponent(entity, componentType);
				}

				matchesCondition &= lookup->hasComponent(entity, t2Type);

				if (matchesCondition)
				{
					m_indices.emplace_back(eastl::make_tuple(i, lookup->getComponentIndex(entity, t2Type)));
				}
			}
		}

		class iterator
		{
			ComponentTable<T1>* m_table1;
			ComponentTable<T2>* m_table2;

			IndicesVector::iterator m_current;
			IndicesVector::iterator m_end;

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = eastl::tuple<T1&, T2&>;
			using difference_type = std::ptrdiff_t;
			using pointer = eastl::tuple<T1&, T2&>*;
			using reference = eastl::tuple<T1&, T2&>&;

			iterator(const IndicesVector::iterator current, const IndicesVector::iterator end,
			         ComponentTable<T1>* table1, ComponentTable<T2>* table2)
				: m_table1(table1), m_table2(table2), m_current(current), m_end(end)
			{
			}

			iterator& operator++()
			{
				++m_current;
				return *this;
			}

			value_type operator*() const
			{
				eastl::tuple<T1&, T2&> tuple = eastl::make_tuple(m_table1->operator[](eastl::get<0>(*m_current)),
				                                                 m_table2->operator[](eastl::get<1>((*m_current))));
				return tuple;
			}

			bool operator==(const iterator& other) const
			{
				return m_current == other.m_current;
			}

			bool operator!=(const iterator& other) const
			{
				return m_current != other.m_current;
			}
		};

	/*	class exclude_inactive_iterator
		{
			iterator m_current;
			iterator m_end;

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = eastl::tuple<T1&, T2&>;
			using difference_type = std::ptrdiff_t;
			using pointer = eastl::tuple<T1&, T2&>*;
			using reference = value_type;

			exclude_inactive_iterator(const iterator& current,
			                          const iterator& end)
				: m_current(current), m_end(end)
			{
				while (m_current != m_end && (!eastl::get<0>(*m_current).isActive || !eastl::get<1>(*m_current).isActive))
				{
					++m_current;
				}
			}

			exclude_inactive_iterator& operator++()
			{
				++m_current;

				while (m_current != m_end && (!eastl::get<0>(*m_current).isActive || !eastl::get<1>(*m_current).isActive))
				{
					++m_current;
				}

				return *this;
			}

			value_type operator*() const
			{
				return *m_current;
			}

			bool operator==(const exclude_inactive_iterator& other) const
			{
				return m_current == other.m_current;
			}

			bool operator!=(const exclude_inactive_iterator& other) const
			{
				return m_current != other.m_current;
			}
		};*/

		iterator begin()
		{
			return iterator(m_indices.begin(), m_indices.end(), m_table1, m_table2);
		}

		iterator end()
		{
			return iterator(m_indices.end(), m_indices.end(), m_table1, m_table2);
		}

	/*	exclude_inactive_iterator exclude_inactive_begin()
		{
			return exclude_inactive_iterator(begin());
		}

		exclude_inactive_iterator exclude_inactive_end()
		{
			return exclude_inactive_iterator(end());
		}

		struct ExcludeInactiveView
		{
			Query2& query;

			auto begin() { return query.exclude_inactive_begin(); }
			auto end() { return query.exclude_inactive_end(); }
		};

		ExcludeInactiveView excludeInactive()
		{
			return ExcludeInactiveView{*this};
		}*/


		~Query2() override = default;
	};

	template <t_plain_component T1, t_plain_component T2, t_plain_component T3>
	class Query3 final : public IQuery
	{
		eastl::vector<sizet, spite::HeapAllocator> m_indices1;
		eastl::vector<sizet, spite::HeapAllocator> m_indices2;
		eastl::vector<sizet, spite::HeapAllocator> m_indices3;

		ComponentTable<T1>* m_table1;
		ComponentTable<T2>* m_table2;
		ComponentTable<T3>* m_table3;

		using TypesVector = eastl::vector<std::type_index, spite::HeapAllocator>;

	public:
		Query3(ComponentLookup* lookup, ComponentStorage* storage,
		       const spite::HeapAllocator& allocator, const TypesVector& hasComponents,
		       const TypesVector& hasNoComponents) : m_indices1(allocator), m_indices2(allocator),
		                                             m_indices3(allocator)
		{
			//if the table is not registered upon query creation
			m_table1 = &storage->getComponentsSafe<T1>();
			m_table2 = &storage->getComponentsSafe<T2>();
			m_table3 = &storage->getComponentsSafe<T3>();
			recreate(lookup, storage, hasComponents, hasNoComponents);
		}

		Query3(const Query3& other) = delete;
		Query3(Query3&& other) = delete;
		Query3& operator=(const Query3& other) = delete;
		Query3& operator=(Query3&& other) = delete;

		sizet getSize() const
		{
			return m_indices1.size();
		}

		sizet getComponentIndexT1(const sizet filterIndex)
		{
			return m_indices1[filterIndex];
		}

		sizet getComponentIndexT2(const sizet filterIndex)
		{
			return m_indices2[filterIndex];
		}

		sizet getComponentIndexT3(const sizet filterIndex)
		{
			return m_indices3[filterIndex];
		}

		T1& getComponentT1(const sizet n)
		{
			return m_table1->operator[](m_indices1[n]);
		}

		T2& getComponentT2(const sizet n)
		{
			return m_table2->operator[](m_indices2[n]);
		}

		T2& getComponentT3(const sizet n)
		{
			return m_table3->operator[](m_indices3[n]);
		}

		void recreate(ComponentLookup* lookup, ComponentStorage* storage, const TypesVector& hasComponents,
		              const TypesVector& hasNoComponents) override
		{
			m_table1 = &storage->getComponentsAsserted<T1>();
			m_table2 = &storage->getComponentsAsserted<T2>();
			m_table3 = &storage->getComponentsAsserted<T3>();

			std::type_index t2Type = std::type_index(typeid(T2));
			std::type_index t3Type = std::type_index(typeid(T3));

			for (sizet i = 0, size = m_table1->getOccupiedSize(); i < size; ++i)
			{
				Entity entity = m_table1->operator[](i).owner;

				bool matchesCondition = true;
				for (const auto& componentType : hasComponents)
				{
					matchesCondition &= lookup->hasComponent(entity, componentType);
				}
				for (const auto& componentType : hasNoComponents)
				{
					matchesCondition &= !lookup->hasComponent(entity, componentType);
				}

				matchesCondition &= lookup->hasComponent(entity, t2Type);
				matchesCondition &= lookup->hasComponent(entity, t3Type);

				if (matchesCondition)
				{
					m_indices1.push_back(i);
					m_indices2.push_back(lookup->getComponentIndex(entity, t2Type));
					m_indices3.push_back(lookup->getComponentIndex(entity, t3Type));
				}
			}
		}



		~Query3() override = default;
	};

	template<t_shared_component T>
	class SharedQuery1 final : public IQuery
	{
		using IndicesVector = eastl::vector<sizet, spite::HeapAllocator>;
		IndicesVector m_indices;

		SharedComponentTable<T>* m_table;

		using TypesVector = eastl::vector<std::type_index, spite::HeapAllocator>;

	public:
		SharedQuery1(ComponentLookup* lookup, ComponentStorage* storage,
		       const spite::HeapAllocator& allocator, const TypesVector& hasComponents,
		       const TypesVector& hasNoComponents) : m_indices(allocator)
		{
			//if the table is not registered upon query creation
			m_table = &storage->getComponentsSafe<T>();
			recreate(lookup, storage, hasComponents, hasNoComponents);
		}

		SharedQuery1(const SharedQuery1& other) = delete;
		SharedQuery1(SharedQuery1&& other) = delete;
		SharedQuery1& operator=(const SharedQuery1& other) = delete;
		SharedQuery1& operator=(SharedQuery1&& other) = delete;

		sizet getSize() const
		{
			return m_indices.size();
		}

		sizet getComponentIndex(const sizet filterIndex)
		{
			return m_indices[filterIndex];
		}

		T& operator[](const sizet n)
		{
			return m_table->operator[](m_indices[n]);
		}

		bool& componentState(const sizet n)
		{
			return m_table->isActive(m_indices[n]);
		}

		std::vector<Entity>& componentOwners(const sizet n)
		{
			return m_table->owners(m_indices[n]);
		}
		
		void recreate(ComponentLookup* lookup, ComponentStorage* storage, const TypesVector& hasComponents,
		              const TypesVector& hasNoComponents) override
		{
			m_indices.clear();
			m_table = &storage->getComponentsAsserted<T>();

			for (sizet i = 0, size = m_table->getOccupiedSize(); i < size; ++i)
			{
				Entity entity = m_table->owner(i);

				bool matchesCondition = true;
				for (const auto& componentType : hasComponents)
				{
					matchesCondition &= lookup->hasComponent(entity, componentType);
				}
				for (const auto& componentType : hasNoComponents)
				{
					matchesCondition &= !lookup->hasComponent(entity, componentType);
				}

				if (matchesCondition)
				{
					m_indices.push_back(i);
				}
			}
		}

		class iterator
		{
			SharedComponentTable<T>* m_table;

			IndicesVector::iterator m_current;
			IndicesVector::iterator m_end;

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			iterator(const IndicesVector::iterator current, const IndicesVector::iterator end, ComponentTable<T>* table)
				: m_table(table), m_current(current), m_end(end)
			{
			}

			iterator& operator++()
			{
				++m_current;
				return *this;
			}

			reference operator*() const
			{
				return m_table->operator[](*m_current);
			}

			bool operator!=(const iterator& other) const
			{
				return m_current != other.m_current;
			}
		};

		class exclude_inactive_iterator
		{
			ComponentTable<T>* m_table;
			IndicesVector::iterator m_current;
			IndicesVector::iterator m_end;

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			exclude_inactive_iterator(const IndicesVector::iterator current,
			                          const IndicesVector::iterator end,
			                          ComponentTable<T>* table)
				: m_table(table), m_current(current), m_end(end)
			{
				// Skip inactive elements 
				while (m_current != m_end && !m_table->isActive(*m_current))
				{
					++m_current;
				}
			}

			exclude_inactive_iterator& operator++()
			{
				++m_current;

				// Skip inactive elements
				while (m_current != m_end && !m_table->isActive(*m_current))
				{
					++m_current;
				}
				return *this;
			}

			exclude_inactive_iterator operator++(int)
			{
				exclude_inactive_iterator tmp = *this;
				++(*this);
				return tmp;
			}

			reference operator*() const
			{
				return m_table->operator[](*m_current);
			}

			pointer operator->() const
			{
				return &(m_table->operator[](*m_current));
			}

			bool operator==(const exclude_inactive_iterator& other) const
			{
				return m_current == other.m_current;
			}

			bool operator!=(const exclude_inactive_iterator& other) const
			{
				return !(*this == other);
			}
		};

		iterator begin()
		{
			return iterator(m_indices.begin(), m_indices.end(), m_table);
		}

		iterator end()
		{
			return iterator(m_indices.end(), m_indices.end(), m_table);
		}

		exclude_inactive_iterator exclude_inactive_begin()
		{
			return exclude_inactive_iterator(m_indices.begin(), m_indices.end(), m_table);
		}

		exclude_inactive_iterator exclude_inactive_end()
		{
			return exclude_inactive_iterator(m_indices.end(), m_indices.end(), m_table);
		}

		struct ExcludeInactiveView
		{
			SharedQuery1& query;

			auto begin() { return query.exclude_inactive_begin(); }
			auto end() { return query.exclude_inactive_end(); }
		};

		ExcludeInactiveView excludeInactive()
		{
			return ExcludeInactiveView{*this};
		}

		~SharedQuery1() override = default;
	};


	class QueryBuildInfo
	{
		eastl::vector<std::type_index, spite::HeapAllocator> m_targetComponents;
		eastl::vector<std::type_index, spite::HeapAllocator> m_hasComponents;
		eastl::vector<std::type_index, spite::HeapAllocator> m_hasNoComponents;

		friend class QueryBuilder;

	public:
		QueryBuildInfo(const spite::HeapAllocator& allocator) : m_targetComponents(allocator),
		                                                        m_hasComponents(allocator),
		                                                        m_hasNoComponents(allocator)
		{
		}

		QueryBuildInfo& hasComponent(const std::type_index typeIndex)
		{
			m_hasComponents.push_back(typeIndex);
			return *this;
		}

		QueryBuildInfo& hasNoComponent(const std::type_index typeIndex)
		{
			m_hasNoComponents.push_back(typeIndex);
			return *this;
		}

		inline friend bool operator==(const QueryBuildInfo& lhs, const QueryBuildInfo& rhs)
		{
			if (!QueryBuildInfo::areEqual(lhs.m_targetComponents, rhs.m_targetComponents))
			{
				return false;
			}
			if (!QueryBuildInfo::areEqual(lhs.m_hasComponents, rhs.m_hasComponents))
			{
				return false;
			}
			if (!QueryBuildInfo::areEqual(lhs.m_hasNoComponents, rhs.m_hasNoComponents))
			{
				return false;
			}
			return true;
		}

		inline friend bool operator!=(const QueryBuildInfo& lhs, const QueryBuildInfo& rhs)
		{
			return !(lhs == rhs);
		}

		bool isDependantOn(const std::type_index type) const
		{
			if (eastl::find(m_targetComponents.begin(), m_targetComponents.end(), type) != m_targetComponents.end())
			{
				return true;
			}
			if (eastl::find(m_hasComponents.begin(), m_hasComponents.end(), type) != m_hasComponents.end())
			{
				return true;
			}
			if (eastl::find(m_hasNoComponents.begin(), m_hasNoComponents.end(), type) != m_hasNoComponents.end())
			{
				return true;
			}
			return false;
		}

		struct hash
		{
			sizet operator()(const QueryBuildInfo& obj) const
			{
				sizet hash = 0;
				for (const auto& component : obj.m_targetComponents)
				{
					hash += component.hash_code();
				}
				for (const auto& component : obj.m_hasComponents)
				{
					hash += component.hash_code();
				}
				for (const auto& component : obj.m_hasNoComponents)
				{
					hash += component.hash_code();
				}
				return hash;
			}
		};

	private:
		static bool areEqual(const eastl::vector<std::type_index, spite::HeapAllocator>& lhs,
		                     const eastl::vector<std::type_index, spite::HeapAllocator>& rhs)
		{
			if (lhs.size() != rhs.size())
			{
				return false;
			}

			for (const auto& elemL : lhs)
			{
				if (eastl::find(rhs.begin(), rhs.end(), elemL) == rhs.end())
				{
					return false;
				}
			}

			return true;
		}
	};


	class QueryBuilder
	{
		std::shared_ptr<ComponentLookup> m_lookup;
		std::shared_ptr<ComponentStorage> m_storage;

		eastl::hash_map<QueryBuildInfo, IQuery*, QueryBuildInfo::hash, eastl::equal_to<QueryBuildInfo>,
		                spite::HeapAllocator> m_queries;

		spite::HeapAllocator m_allocator;

	public:
		QueryBuilder(std::shared_ptr<ComponentLookup> lookup, std::shared_ptr<ComponentStorage> storage,
		             const spite::HeapAllocator& allocator)
			: m_lookup(std::move(lookup)),
			  m_storage(std::move(storage)),
			  m_queries(allocator),
			  m_allocator(allocator)
		{
		}


		QueryBuildInfo getQueryBuildInfo() const
		{
			return QueryBuildInfo(m_allocator);
		}

		template <t_plain_component T>
		Query1<T>* buildQuery(QueryBuildInfo& buildInfo)
		{
			buildInfo.m_targetComponents.push_back(std::type_index(typeid(T)));

			if (contains(buildInfo))
			{
				STEST_LOG_UNPRINTED(TESTLOG_ECS_QUERY_LOADED_FROM_CACHE(typeid(T).name()))
				IQuery* query = m_queries.at(buildInfo);
				return static_cast<Query1<T>*>(query);
			}

			STEST_LOG_UNPRINTED(TESTLOG_ECS_NEW_QUERY_CREATED(typeid(T).name()))
			IQuery* query = new Query1<T>(m_lookup.get(), m_storage.get(), m_allocator, buildInfo.m_hasComponents,
			                              buildInfo.m_hasNoComponents);
			m_queries.emplace(buildInfo, query);
			return static_cast<Query1<T>*>(query);
		}

		template <t_plain_component T1, t_plain_component T2>
		Query2<T1, T2>* buildQuery(QueryBuildInfo& buildInfo)
		{
			buildInfo.m_targetComponents.push_back(std::type_index(typeid(T1)));
			buildInfo.m_targetComponents.push_back(std::type_index(typeid(T2)));

			if (contains(buildInfo))
			{
				STEST_LOG_UNPRINTED(
					TESTLOG_ECS_QUERY_LOADED_FROM_CACHE((std::string(typeid(T1).name()) + std::string(typeid(T2).name())
					).c_str()))
				IQuery* query = m_queries.at(buildInfo);
				return static_cast<Query2<T1, T2>*>(query);
			}

			STEST_LOG_UNPRINTED(
				TESTLOG_ECS_NEW_QUERY_CREATED((std::string(typeid(T1).name()) + std::string(typeid(T2).name())).c_str()
				))
			IQuery* query = new Query2<T1, T2>(m_lookup.get(), m_storage.get(), m_allocator, buildInfo.m_hasComponents,
			                                   buildInfo.m_hasNoComponents);
			m_queries.emplace(buildInfo, query);
			return static_cast<Query2<T1, T2>*>(query);
		}

		template <t_plain_component T1, t_plain_component T2, t_plain_component T3 >
		Query3<T1, T2, T3>* buildQuery(QueryBuildInfo& buildInfo)
		{
			buildInfo.m_targetComponents.push_back(std::type_index(typeid(T1)));
			buildInfo.m_targetComponents.push_back(std::type_index(typeid(T2)));
			buildInfo.m_targetComponents.push_back(std::type_index(typeid(T3)));

			if (contains(buildInfo))
			{
				IQuery* query = m_queries.at(buildInfo);
				STEST_LOG_UNPRINTED(
					TESTLOG_ECS_QUERY_LOADED_FROM_CACHE((std::string(typeid(T1).name()) + std::string(typeid(T2).name())
						+ std::string(typeid(T3).name())).c_str()))
				return static_cast<Query3<T1, T2, T3>*>(query);
			}

			STEST_LOG_UNPRINTED(
				TESTLOG_ECS_NEW_QUERY_CREATED((std::string(typeid(T1).name()) + std::string(typeid(T2).name())+ std::
					string(typeid(T3).name())).c_str()))
			IQuery* query = new Query3<T1, T2, T3>(m_lookup.get(), m_storage.get(), m_allocator,
			                                       buildInfo.m_hasComponents,
			                                       buildInfo.m_hasNoComponents);
			m_queries.emplace(buildInfo, query);
			return static_cast<Query3<T1, T2, T3>*>(query);
		}

		template <t_shared_component T>
		SharedQuery1<T>* buildQuery(QueryBuildInfo& buildInfo)
		{
			buildInfo.m_targetComponents.push_back(std::type_index(typeid(T)));

			if (contains(buildInfo))
			{
				STEST_LOG_UNPRINTED(TESTLOG_ECS_QUERY_LOADED_FROM_CACHE(typeid(T).name()))
				IQuery* query = m_queries.at(buildInfo);
				return static_cast<SharedQuery1<T>*>(query);
			}

			STEST_LOG_UNPRINTED(TESTLOG_ECS_NEW_QUERY_CREATED(typeid(T).name()))
			IQuery* query = new SharedQuery1<T>(m_lookup.get(), m_storage.get(), m_allocator, buildInfo.m_hasComponents,
			                              buildInfo.m_hasNoComponents);
			m_queries.emplace(buildInfo, query);
			return static_cast<SharedQuery1<T>*>(query);
		}

		void recreateDependentQueries(const std::type_index type)
		{
			for (const auto& keyval : m_queries)
			{
				if (keyval.first.isDependantOn(type))
				{
					keyval.second->recreate(m_lookup.get(), m_storage.get(), keyval.first.m_hasComponents,
					                        keyval.first.m_hasNoComponents);
				}
			}
		}

		~QueryBuilder()
		{
			for (const auto& keyval : m_queries)
			{
				delete keyval.second;
			}
		}

	private:
		bool contains(const QueryBuildInfo& buildInfo) const
		{
			return m_queries.find(buildInfo) != m_queries.end();
		}
	};
}
