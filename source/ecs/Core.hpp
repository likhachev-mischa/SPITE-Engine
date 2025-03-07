#pragma once
#include <any>
#include <memory>
#include <typeindex>

#include <EASTL/hash_map.h>
#include <EASTL/vector.h>

#include "base/Assert.hpp"
#include "base/Event.hpp"
#include "base/Logging.hpp"
#include "base/Memory.hpp"
#include "base/Platform.hpp"

namespace spite
{
	class IStructuralChangeHandler;
	typedef HeapAllocator ComponentAllocator;

	struct Entity
	{
	private:
		u64 m_id;

	public:
		explicit Entity(const u64 id = 0);
		u64 id() const;
		friend bool operator==(const Entity& lhs, const Entity& rhs);
		friend bool operator!=(const Entity& lhs, const Entity& rhs);

		struct hash
		{
			size_t operator()(const spite::Entity& entity) const;
		};
	};

	struct IComponent
	{
		IComponent() = default;
		IComponent(const IComponent& other) = default;
		IComponent(IComponent&& other) noexcept = default;
		IComponent& operator=(const IComponent& other) = default;
		IComponent& operator=(IComponent&& other) noexcept = default;
		virtual ~IComponent() = default;
	};

	struct ISharedComponent : IComponent
	{
	};

	struct ISingletonComponent : IComponent
	{
	};

	struct IEventComponent : IComponent
	{
	};

	//components in general
	template <typename TComponent>
	concept t_component = std::is_base_of_v<IComponent, TComponent> && std::is_default_constructible_v<TComponent> &&
		std::is_copy_constructible_v<TComponent> &&
		std::is_move_assignable_v<TComponent> && std::is_move_constructible_v<TComponent>;

	//specificly only IComponent
	template <typename TComponent>
	concept t_plain_component = std::is_base_of_v<IComponent, TComponent> && !std::is_base_of_v<
			ISharedComponent, TComponent> && !std::is_base_of_v<ISingletonComponent, TComponent> &&
		std::is_default_constructible_v<TComponent> &&
		std::is_copy_constructible_v<TComponent> &&
		std::is_move_assignable_v<TComponent> && std::is_move_constructible_v<TComponent>;

	//specificly ISharedComponent
	template <typename TComponent>
	concept t_shared_component = std::is_base_of_v<ISharedComponent, TComponent> && !std::is_base_of_v<
			ISingletonComponent, TComponent> && std::is_default_constructible_v<
			TComponent> &&
		std::is_copy_constructible_v<TComponent> &&
		std::is_move_assignable_v<TComponent> && std::is_move_constructible_v<TComponent>;

	//specificly ISingletonComponent
	template <typename TComponent>
	concept t_singleton_component = std::is_base_of_v<ISingletonComponent, TComponent> && !std::is_base_of_v<
			ISharedComponent, TComponent> && std::is_default_constructible_v<
			TComponent> &&
		std::is_copy_constructible_v<TComponent> &&
		std::is_move_assignable_v<TComponent> && std::is_move_constructible_v<TComponent>;

	//specificly IEventComponent
	template <typename TComponent>
	concept t_event_component = std::is_base_of_v<IEventComponent, TComponent> && !std::is_base_of_v<
			ISharedComponent, TComponent> && !std::is_base_of_v<ISingletonComponent, TComponent> &&
		std::is_default_constructible_v<
			TComponent> &&
		std::is_copy_constructible_v<TComponent> &&
		std::is_move_assignable_v<TComponent> && std::is_move_constructible_v<TComponent>;

	//interface for component vector usage when type is resolved in runtime
	//any casting should be avoided whenever possible
	class IComponentProvider
	{
	public:
		//returns true if structural change is required
		virtual bool removeComponent(const Entity owner, sizet idx) = 0;

		virtual bool isEmpty() = 0;
		//should return an array even if there is no/one entity
		virtual Entity* getTopEntities(sizet& n) = 0;
		virtual ~IComponentProvider() = default;
	};

	template <typename T>
	concept t_movable = std::is_copy_constructible_v<T> && std::is_move_assignable_v<T> && std::is_move_constructible_v<
		T>;


	template <t_movable T>
	class PooledVector
	{
	protected:
		using VectorType = eastl::vector<T, spite::HeapAllocator>;
		VectorType m_vector;

		//index of occupied top element
		int m_topIdx;

	public:
		explicit PooledVector(const spite::HeapAllocator& allocator, const sizet initialSize = 10) :
			m_vector(allocator)
		{
			m_vector.reserve(initialSize);
			m_topIdx = -1;
		}

		PooledVector(const PooledVector& other) : m_vector(other.m_vector), m_topIdx(other.m_topIdx)
		{
		}

		PooledVector(PooledVector&& other) noexcept :
			m_vector(std::move(other.m_vector)),
			m_topIdx(std::move(other.m_topIdx))
		{
		}

		//just moves top pointer: no reallocations, but the table is free
		void rewind()
		{
			m_topIdx = -1;
		}

		//returns -1 if vector is empty
		int getTopIndex() const
		{
			return m_topIdx;
		}

		sizet getOccupiedSize() const
		{
			//SASSERTM(m_topIdx >= -1, "Vector is uninitialized")
			return (m_topIdx + 1);
		}

		sizet getTotalSize()
		{
			return m_vector.size();
		}

		sizet getCapacity()
		{
			return m_vector.capacity();
		}

		//occupied slots / all slots
		float getFillFactor()
		{
			return getOccupiedSize() / static_cast<float>(m_vector.capacity());
		}

		void reserve(sizet n)
		{
			m_vector.reserve(n);
		}

		void setCapacity(sizet n)
		{
			SASSERTM(n > m_topIdx, "SET CAPACITY WILL DELETE STORED ELEMENTS");
			m_vector.set_capacity(n);
		}

		//preferable to use this
		//WARNING: uses move assignment/insertion for passed params
		template <typename InputIterator>
		void addElements(InputIterator& begin, InputIterator& end)
		{
			auto destIter = m_vector.begin() + m_topIdx + 1;
			auto srcIter = begin;

			//if there is previously initialized but unused space in vector
			for (sizet i = m_topIdx + 1, size = m_vector.size(); i < size && srcIter != end; ++i, ++destIter, ++
			     srcIter, ++m_topIdx)
			{
				*destIter = std::move(*srcIter);
				++destIter;
				++srcIter;
			}
			if (srcIter != end)
			{
				sizet resizeCount = std::distance(srcIter, end);;
				SDEBUG_LOG("pooled vector of type %s: insert %llu elems, fill factor: %f, total size: %llu\n",
				           std::type_index(typeid(T)).name(), resizeCount,
				           getFillFactor(), m_vector.size());
				m_vector.insert(destIter, eastl::make_move_iterator(srcIter), eastl::make_move_iterator(end));
				m_topIdx += static_cast<int>(resizeCount);
			}
		}

		void merge(PooledVector<T>& other)
		{
			auto begin = other.begin();
			auto end = other.end();
			addElements(begin, end);
		}

		// use bulk insertion instead if possible
		void addElement(T element)
		{
			++m_topIdx;
			if (static_cast<sizet>(m_topIdx) < m_vector.size())
			{
				m_vector[m_topIdx] = std::move(element);
			}
			else
			{
				m_vector.emplace_back(std::move(element));
			}
		}

		/**
		 * \brief simply replaces elements on input indices with elements 
		 * from the top of the table and decreases top pointer
		 * \param array : an array of indices
		 * WARNING: all indices must be unique - undefined behaviour otherwise
		 * \param n : array size
		 */
		void removeElements(const sizet* array, const sizet n)
		{
			SASSERTM(m_topIdx >= n - 1, "TRYING TO REMOVE MORE elements THAN PRESENT");
			for (sizet i = 0; i < n; ++i)
			{
				sizet idx = array[i];
				m_vector[idx] = std::move(m_vector[m_topIdx]);
				--m_topIdx;
			}
		}

		void removeElement(sizet idx)
		{
			SASSERTM(m_topIdx >= 0, "TRYING TO REMOVE ELEMENT IN EMPTY VECTOR")
			m_vector[idx] = std::move(m_vector[m_topIdx]);
			--m_topIdx;
		}

		//eastl vector should assert its bounds
		T& operator[](sizet n)
		{
			return m_vector[n];
		}

		typename VectorType::iterator begin()
		{
			return m_vector.begin();
		}

		typename VectorType::iterator end()
		{
			return m_vector.end();
		}

		typename VectorType::const_iterator cbegin() const
		{
			return m_vector.cbegin();
		}

		typename VectorType::const_iterator cend() const
		{
			return m_vector.cend();
		}
	};

	//all vector operations are wrappers around PooledVector methods 
	template <t_plain_component TComponent>
	class ComponentTable final : public IComponentProvider
	{
		//true - component enabled, false - disabled
		PooledVector<bool> m_componentsStatuses;
		PooledVector<Entity> m_componentsOwners;

		PooledVector<TComponent> m_components;

	public:
		explicit ComponentTable(const spite::HeapAllocator& allocator, const sizet initialSize = 10);

		ComponentTable(const ComponentTable& other) = delete;
		ComponentTable(ComponentTable&& other) = delete;
		ComponentTable& operator=(ComponentTable&& other) = delete;
		ComponentTable& operator=(const ComponentTable& other) = delete;

		void rewind()
		{
			m_componentsStatuses.rewind();
			m_componentsOwners.rewind();
			m_components.rewind();
		}

		void reserve(sizet n)
		{
			m_componentsStatuses.reserve(n);
			m_componentsOwners.reserve(n);
			m_components.reserve(n);
		}

		void setCapacity(sizet n)
		{
			m_componentsStatuses.setCapacity(n);
			m_componentsOwners.setCapacity(n);
			m_components.setCapacity(n);
		}

		int getTopIndex()
		{
			return m_components.getTopIndex();
		}

		sizet getCapacity()
		{
			return m_components.getCapacity();
		}

		sizet getOccupiedSize()
		{
			return m_components.getOccupiedSize();
		}

		sizet getTotalSize()
		{
			return m_components.getTotalSize();
		}

		float getFillFactor()
		{
			return m_components.getFillFactor();
		}

		//sets component's active status
		void setActive(const sizet idx, const bool value)
		{
			m_componentsStatuses[idx] = value;
		}

		void addComponent(TComponent component, const Entity owner, bool isActive = true)
		{
			m_componentsStatuses.addElement(isActive);
			m_componentsOwners.addElement(owner);
			m_components.addElement(component);
		}

		//WARNING: avoid duplications manually
		void addComponents(ComponentTable<TComponent>& other)
		{
			m_componentsStatuses.merge(other.m_componentsStatuses);
			m_componentsOwners.merge(other.m_componentsOwners);
			m_components.merge(other.m_components);
		}

		void removeComponent(sizet idx)
		{
			m_componentsStatuses.removeElement(idx);
			m_componentsOwners.removeElement(idx);
			m_components.removeElement(idx);
		}

		bool removeComponent(const Entity owner, sizet idx) override
		{
			removeComponent(idx);
			return true;
		}

		void removeComponents(sizet* array, const sizet n)
		{
			m_componentsStatuses.removeElements(array, n);
			m_componentsOwners.removeElements(array, n);
			m_components.removeElements(array, n);
		}

		bool isEmpty() override
		{
			return m_components.getOccupiedSize() == 0;
		}

		Entity* getTopEntities(sizet& n) override
		{
			n = 1;
			return &m_componentsOwners[m_components.getTopIndex()];
		}

		Entity owner(sizet idx)
		{
			return m_componentsOwners[idx];
		}

		bool isActive(sizet idx)
		{
			return m_componentsStatuses[idx];
		}

		TComponent& operator[](sizet n)
		{
			return m_components[n];
		}

		~ComponentTable() override = default;
	};

	//intentionally does not inherit from ComponentTable, but replicates some functionality
	template <t_shared_component T>
	class SharedComponentTable : public IComponentProvider
	{
		PooledVector<bool> m_componentsStatuses;
		PooledVector<std::vector<Entity>> m_componentsOwners;

		PooledVector<T> m_components;

	public:
		explicit SharedComponentTable(const spite::HeapAllocator& allocator, const sizet initialSize = 10):
			m_componentsStatuses(allocator, initialSize), m_componentsOwners(allocator, initialSize),
			m_components(allocator, initialSize)
		{
		}

		SharedComponentTable(const SharedComponentTable& other) = delete;
		SharedComponentTable(SharedComponentTable&& other) = delete;
		SharedComponentTable& operator=(SharedComponentTable&& other) = delete;
		SharedComponentTable& operator=(const SharedComponentTable& other) = delete;

		void rewind()
		{
			m_componentsStatuses.rewind();
			m_componentsOwners.rewind();
			m_components.rewind();
		}

		void reserve(sizet n)
		{
			m_componentsStatuses.reserve(n);
			m_componentsOwners.reserve(n);
			m_components.reserve(n);
		}

		void setCapacity(sizet n)
		{
			m_componentsStatuses.setCapacity(n);
			m_componentsOwners.setCapacity(n);
			m_components.setCapacity(n);
		}

		int getTopIndex()
		{
			return m_components.getTopIndex();
		}

		sizet getCapacity()
		{
			return m_components.getCapacity();
		}

		sizet getOccupiedSize()
		{
			return m_components.getOccupiedSize();
		}

		sizet getTotalSize()
		{
			return m_components.getTotalSize();
		}

		float getFillFactor()
		{
			return m_components.getFillFactor();
		}

		//sets component's active status
		void setActive(const sizet idx, const bool value)
		{
			m_componentsStatuses[idx] = value;
		}

		bool hasOwner(const Entity entity)
		{
			for (auto& owners : m_componentsOwners)
			{
				if (eastl::find(owners.begin(), owners.end(), entity) != owners.end())
				{
					return true;
				}
			}
			return false;
		}

		//adds entity to already existing shared component
		void addComponent(const Entity entity, const sizet idx)
		{
			SASSERTM(!hasOwner(entity), "Entity %llu already owns component %s", entity.id(), typeid(T).name());
			m_componentsOwners[idx].push_back(entity);
		}

		//creates new SharedComponent instance
		void addComponent(T component, const Entity owner, bool isActive = true)
		{
			m_componentsStatuses.addElement(isActive);
			m_componentsOwners.addElement({owner});
			m_components.addElement(component);
		}

		//removes owner of unspecified component
		//returns true if it was the last owner - meaning that structural change should be performed
		//false otherwise
		bool removeComponent(const Entity entity)
		{
			SASSERTM(hasOwner(entity), "Entity %llu doesnt own component %s", entity.id(), typeid(T).name());
			for (sizet i = 0, size = getOccupiedSize(); i < size; ++i)
			{
				auto& owners = m_componentsOwners[i];
				auto iter = eastl::find(owners.begin(), owners.end(), entity);
				if (iter != owners.end())
				{
					owners.erase(iter);
					if (owners.size() == 0)
					{
						deleteComponent(i);
						return true;
					}
					return false;
				}
			}
			return false;
		}

		//removes owner of specified component
		//returns true if it was the last owner - meaning that structural change should be performed
		//false otherwise
		bool removeComponent(const Entity entity, const sizet idx) override
		{
			auto& owners = m_componentsOwners[idx];
			auto iter = eastl::find(owners.begin(), owners.end(), entity);
			SASSERTM(iter != owners.end(), "Entity %llu doesnt own component %s", entity.id(), typeid(T).name());
			owners.erase(iter);

			if (owners.size() == 0)
			{
				deleteComponent(idx);
				return true;
			}
			return false;
		}

		//deletes component ignoring other entities' ownership 
		void deleteComponent(sizet idx)
		{
			m_componentsStatuses.removeElement(idx);
			m_componentsOwners.removeElement(idx);
			m_components.removeElement(idx);
		}

		//deletes components ignoring other entities' ownership 
		void deleteComponents(sizet* array, const sizet n)
		{
			m_componentsStatuses.removeElements(array, n);
			m_componentsOwners.removeElements(array, n);
			m_components.removeElements(array, n);
		}

		bool isEmpty() override
		{
			return m_components.getOccupiedSize() == 0;
		}

		Entity* getTopEntities(sizet& n) override
		{
			auto& entities = owners(getTopIndex());
			n = entities.size();
			return entities.data();
		}

		std::vector<Entity>& owners(sizet idx)
		{
			return m_componentsOwners[idx];
		}

		bool isActive(sizet idx)
		{
			return m_componentsStatuses[idx];
		}

		T& operator[](sizet n)
		{
			return m_components[n];
		}

		~SharedComponentTable() override = default;
	};

	struct ISingletonTable
	{
	};

	//holds only one component of type
	template <t_singleton_component T>
	class SingletonComponentTable : ISingletonTable
	{
		T m_component;

	public:
		SingletonComponentTable() = default;

		SingletonComponentTable(T component) : m_component(std::move(component))
		{
		}

		SingletonComponentTable(const SingletonComponentTable& other) = delete;
		SingletonComponentTable(SingletonComponentTable&& other) noexcept = delete;
		SingletonComponentTable& operator=(const SingletonComponentTable& other) = delete;
		SingletonComponentTable& operator=(SingletonComponentTable&& other) noexcept = delete;

		void setComponentData(T component)
		{
			m_component = std::move(component);
		}

		T& getComponent()
		{
			return m_component;
		}

		~SingletonComponentTable() = default;
	};

	struct IEventTable
	{
		virtual bool isEmpty() = 0;
		virtual void rewind() = 0;
		virtual ~IEventTable() = default;
	};

	template <t_event_component T>
	class EventComponentTable : IEventTable
	{
		PooledVector<T> m_events;

	public:
		EventComponentTable(const HeapAllocator& allocator, const sizet initialCount = 10): m_events(
			allocator, initialCount)
		{
		}

		sizet getSize()
		{
			return m_events.getOccupiedSize();
		}

		bool isEmpty() override
		{
			return m_events.getOccupiedSize() == 0;
		}

		void rewind()
		{
			m_events.rewind();
		}

		void addEvent(T event)
		{
			m_events.addElement(std::move(event));
		}

		T& operator[](sizet n)
		{
			return m_events[n];
		}

		~EventComponentTable() override = default;
	};

	class ComponentStorage
	{
		eastl::hash_map<std::type_index, IComponentProvider*, std::hash<std::type_index>, eastl::equal_to<
			                std::type_index>, ComponentAllocator> m_storage;

		eastl::hash_map<std::type_index, IEventTable*, std::hash<std::type_index>, eastl::equal_to<
			                std::type_index>, ComponentAllocator> m_eventStorage;

		eastl::hash_map<std::type_index, ISingletonTable*, std::hash<std::type_index>, eastl::equal_to<
			                std::type_index>, ComponentAllocator> m_singletonStorage;

		ComponentAllocator m_componentAllocator;

	public:
		explicit ComponentStorage(const ComponentAllocator& componentAllocator) :
			m_storage(componentAllocator),
			m_componentAllocator(componentAllocator)
		{
		}

		template <t_plain_component TComponent>
		void registerComponent()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(!isComponentRegistred(typeIndex), "Component of type %s is already registered", typeIndex.name());

			IComponentProvider* provider = new ComponentTable<TComponent>(m_componentAllocator);
			m_storage.emplace(typeIndex, provider);
		}

		template <t_shared_component TComponent>
		void registerComonent()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(!isComponentRegistred(typeIndex), "Component of type %s is already registered", typeIndex.name());

			IComponentProvider* provider = new SharedComponentTable<TComponent>(m_componentAllocator);
			m_storage.emplace(typeIndex, provider);
		}

		template <t_event_component T>
		void registerEvent()
		{
			std::type_index typeIndex = std::type_index(typeid(T));
			SASSERTM(!isComponentRegistred(typeIndex), "Event of type %s is already registered", typeIndex.name());

			IEventTable* table = new EventComponentTable<T>(m_componentAllocator);
			m_eventStorage.emplace(typeIndex, table);
		}

		template <t_singleton_component TComponent>
		void createSingleton()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(!isSingletonCreated(typeIndex), "Singleton of type %s is already created", typeIndex.name());

			ISingletonTable* table = new SingletonComponentTable<TComponent>();
			m_storage.emplace(typeIndex, table);
		}

		bool isComponentRegistred(const std::type_index typeIndex)
		{
			auto iterator = m_storage.find(typeIndex);
			return iterator != m_storage.end();
		}

		bool isSingletonCreated(const std::type_index typeIndex)
		{
			auto iterator = m_singletonStorage.find(typeIndex);
			return iterator != m_singletonStorage.end();
		}

		bool isEventRegistred(const std::type_index typeIndex)
		{
			auto iterator = m_eventStorage.find(typeIndex);
			return iterator != m_eventStorage.end();
		}

		template <t_component TComponent>
		void unregisterComponent()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(isComponentRegistred(typeIndex), "Component %s was not present in storage to unregister it",
			         typeIndex.name());

			delete m_storage.at(typeIndex);
		}

		template <t_singleton_component TComponent>
		void deleteSingleton()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(isSingletonCreated(typeIndex), "Singleton %s was not present in storage to delete it",
			         typeIndex.name());

			delete m_singletonStorage.at(typeIndex);
		}

		template <t_event_component T>
		void unregisterEvent()
		{
			std::type_index typeIndex = std::type_index(typeid(T));
			SASSERTM(isEventRegistred(typeIndex), "Event %s was not present in storage to delete it",
			         typeIndex.name());

			delete m_eventStorage.at(typeIndex);
		}

		//returns true if at least one component of type is present
		bool hasAnyComponents(const std::type_index typeIndex)
		{
			bool result;
			IComponentProvider* provider = getRawProviderNullable(typeIndex);
			if (provider == nullptr || provider->isEmpty())
			{
				result = false;
			}
			else
			{
				return true;
			}
			if (isEventRegistred(typeIndex) && !getRawEventTable(typeIndex).isEmpty())
			{
				return true;
			}

			result |= isSingletonCreated(typeIndex);
			return result;
		}

		IComponentProvider& getRawProviderAsserted(const std::type_index typeIndex)
		{
			SASSERTM(isComponentRegistred(typeIndex), "No components of type %s exist in storage", typeIndex.name())
			return *m_storage.at(typeIndex);
		}

		IComponentProvider* getRawProviderNullable(const std::type_index typeIndex)
		{
			if (!isComponentRegistred(typeIndex))
			{
				return nullptr;
			}

			return m_storage.at(typeIndex);
		}

		IEventTable& getRawEventTable(const std::type_index typeIndex)
		{
			SASSERTM(isEventRegistred(typeIndex), "Event %s is not registred", typeIndex.name());
			return *m_eventStorage.at(typeIndex);
		}

		template <t_plain_component TComponent>
		ComponentTable<TComponent>& getComponentsAsserted()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(isComponentRegistred(typeIndex), "No components of type %s exist in storage", typeIndex.name());

			auto& value = m_storage.at(typeIndex);
			auto table = reinterpret_cast<ComponentTable<TComponent>*>(value);
			return *table;
		}

		template <t_shared_component TComponent>
		SharedComponentTable<TComponent>& getComponentsAsserted()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(isComponentRegistred(typeIndex), "No components of type %s exist in storage", typeIndex.name());

			auto& value = m_storage.at(typeIndex);
			auto table = reinterpret_cast<SharedComponentTable<TComponent>*>(value);
			return *table;
		}

		template <t_event_component T>
		EventComponentTable<T>& getEventsAsserted()
		{
			std::type_index typeIndex = std::type_index(typeid(T));
			SASSERTM(isEventRegistred(typeIndex), "No event of type %s exist in storage", typeIndex.name());

			auto& value = m_eventStorage.at(typeIndex);
			auto table = reinterpret_cast<EventComponentTable<T>*>(value);
			return *table;
		}

		template <t_component TComponent>
		ComponentTable<TComponent>& getComponentsSafe()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			if (!isComponentRegistred(typeIndex))
			{
				registerComponent<TComponent>();
			}
			auto& value = m_storage.at(typeIndex);
			auto table = reinterpret_cast<ComponentTable<TComponent>*>(value);
			return *table;
		}

		template <t_shared_component TComponent>
		SharedComponentTable<TComponent>& getComponentsSafe()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			if (!isComponentRegistred(typeIndex))
			{
				registerComponent<TComponent>();
			}
			auto& value = m_storage.at(typeIndex);
			auto table = reinterpret_cast<SharedComponentTable<TComponent>*>(value);
			return *table;
		}

		template <t_event_component T>
		EventComponentTable<T>& getEventsSafe()
		{
			std::type_index typeIndex = std::type_index(typeid(T));
			if (!isEventRegistred(typeIndex))
			{
				registerEvent<T>();
			}

			auto& value = m_eventStorage.at(typeIndex);
			auto table = reinterpret_cast<EventComponentTable<T>*>(value);
			return *table;
		}

		template <t_singleton_component TComponent>
		SingletonComponentTable<TComponent>& getSingleton()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(isSingletonCreated(typeIndex), "Singleton of type %s is not present in storage", typeIndex.name());

			auto& value = m_singletonStorage.at(typeIndex);
			auto table = reinterpret_cast<SingletonComponentTable<TComponent>*>(value);
			return *table;
		}

		~ComponentStorage()
		{
			for (const auto& pair : m_storage)
			{
				delete pair.second;
			}
			m_storage.clear(true);

			for (const auto& pair : m_singletonStorage)
			{
				delete pair.second;
			}
			m_singletonStorage.clear(true);
			for (const auto& pair : m_eventStorage)
			{
				delete pair.second;
			}
			m_eventStorage.clear(true);
		}
	};

	struct LookupData
	{
		std::type_index type;
		sizet index;

		LookupData(const std::type_index type, const sizet index);
		LookupData(LookupData&& other) noexcept = default;
		LookupData(const LookupData& other) = default;
		LookupData& operator=(const LookupData& other) = default;
		LookupData& operator=(LookupData&& other) noexcept = default;
		~LookupData() = default;
	};

	class ComponentLookup
	{
		eastl::hash_map<Entity, PooledVector<LookupData>, Entity::hash, eastl::equal_to<
			                Entity>, spite::HeapAllocator> m_lookup;
		spite::HeapAllocator m_allocator;

	public:
		ComponentLookup(const spite::HeapAllocator& allocator);

		void trackEntity(const Entity entity);

		void untrackEntity(const Entity entity);

		bool isEntityTracked(const Entity entity);

		PooledVector<LookupData>& getLookupData(const Entity entity);

		bool hasComponent(const Entity entity, const std::type_index typeIndex);

		sizet getComponentIndex(const Entity entity, const std::type_index typeIndex);

		void addComponentToLookup(const Entity entity, const std::type_index typeIndex, const sizet componentIndex);

		void removeComponentFromLookup(const Entity entity, const std::type_index typeIndex);

		void setComponentIndex(const Entity entity, const std::type_index typeIndex, const sizet newIndex);

	private:
		sizet getComponentLookupIndex(const Entity entity, const std::type_index typeIndex);
	};

	class IStructuralChangeHandler
	{
	public:
		virtual void handleStructuralChange(const std::type_index& typeIndex) = 0;
		virtual ~IStructuralChangeHandler() = default;
	};

	class ComponentManager
	{
		std::shared_ptr<ComponentStorage> m_storage;
		std::shared_ptr<ComponentLookup> m_lookup;

		std::shared_ptr<IStructuralChangeHandler> m_structuralChangeHandler;

	public:
		ComponentManager(std::shared_ptr<ComponentStorage> componentStorage,
		                 std::shared_ptr<ComponentLookup> componentLookup,
		                 std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler) :
			m_storage(std::move(componentStorage)),
			m_lookup(std::move(componentLookup)),
			m_structuralChangeHandler(std::move(structuralChangeHandler))
		{
		}

		//note: any changes to passed component param won't be saved in storage after this func
		template <t_plain_component TComponent>
		void addComponent(Entity entity, TComponent& component, bool isActive = true)
		{
			auto& components = m_storage->getComponentsSafe<TComponent>();
			components.addComponent(component, entity, isActive);
			sizet componentIndex = components.getTopIndex();

			std::type_index typeIndex = std::type_index(typeid(TComponent));
			m_lookup->addComponentToLookup(entity, typeIndex, componentIndex);

			m_structuralChangeHandler->handleStructuralChange(typeIndex);
		}

		template <t_plain_component TComponent>
		void addComponent(Entity entity, bool isActive = true)
		{
			TComponent component;

			auto& components = m_storage->getComponentsSafe<TComponent>();
			components.addComponent(component, entity, isActive);
			sizet componentIndex = components.getTopIndex();

			std::type_index typeIndex = std::type_index(typeid(TComponent));
			m_lookup->addComponentToLookup(entity, typeIndex, componentIndex);

			m_structuralChangeHandler->handleStructuralChange(typeIndex);
		}

		//creates a new SharedComponent instance
		template <t_shared_component TComponent>
		void addComponent(const Entity entity, TComponent& component, bool isActive = true)
		{
			auto& components = m_storage->getComponentsSafe<TComponent>();
			components.addComponent(component, entity, isActive);
			sizet componentIndex = components.getTopIndex();

			std::type_index typeIndex = std::type_index(typeid(TComponent));
			m_lookup->addComponentToLookup(entity, typeIndex, componentIndex);

			m_structuralChangeHandler->handleStructuralChange(typeIndex);
		}


		//adds target entity as owner of source's SharedComponent
		template <t_shared_component TComponent>
		void addComponent(const Entity source, const Entity target)
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			sizet componentIndex = m_lookup->getComponentIndex(source, typeIndex);

			auto& components = m_storage->getComponentsAsserted<TComponent>();
			components.addComponent(target, componentIndex);

			m_lookup->addComponentToLookup(target, typeIndex, componentIndex);

			//probably shouldn't invoke structural change
			//m_structuralChangeHandler->handleStructuralChange(typeIndex);
		}

		template <t_plain_component TComponent>
		void removeComponent(const Entity entity)
		{
			auto& componentTable = m_storage->getComponentsAsserted<TComponent>();

			std::type_index typeIndex = std::type_index(typeid(TComponent));

			sizet index = m_lookup->getComponentIndex(entity, typeIndex);
			Entity topEntity = componentTable.owner(componentTable.getTopIndex());

			m_lookup->setComponentIndex(topEntity, typeIndex, index);

			m_lookup->removeComponentFromLookup(entity, typeIndex);

			componentTable.removeComponent(index);

			m_structuralChangeHandler->handleStructuralChange(typeIndex);
		}

		template <t_shared_component TComponent>
		void removeComponent(const Entity entity)
		{
			auto& componentTable = m_storage->getComponentsAsserted<TComponent>();

			std::type_index typeIndex = std::type_index(typeid(TComponent));

			sizet index = m_lookup->getComponentIndex(entity, typeIndex);

			auto& topEntities = componentTable.owners(componentTable.getTopIndex());
			for (auto topEntity : topEntities)
			{
				m_lookup->setComponentIndex(topEntity, typeIndex, index);
			}

			m_lookup->removeComponentFromLookup(entity, typeIndex);

			bool isStructuralChange = componentTable.removeComponent(entity, index);

			if (isStructuralChange)
			{
				m_structuralChangeHandler->handleStructuralChange(typeIndex);
			}
		}

		template <t_singleton_component TComponent>
		void createSingleton(TComponent component)
		{
			m_storage->createSingleton<TComponent>(std::move(component));
		}

		template <t_singleton_component TComponent>
		void deleteSingleton()
		{
			m_storage->deleteSingleton<TComponent>();
		}

		template <t_singleton_component TComponent>
		TComponent& getSingleton()
		{
			return m_storage->getSingleton<TComponent>();
		}

		template <t_singleton_component TComponent>
		bool isSingletonExists() const
		{
			return m_storage->isSingletonCreated(typeid(TComponent));
		}

		//asserts that entity has component
		template <typename TComponent>
			requires t_plain_component<TComponent> || t_shared_component<TComponent>
		TComponent& getComponent(const Entity entity)
		{
			sizet idx = m_lookup->getComponentIndex(entity, std::type_index(typeid(TComponent)));
			return m_storage->getComponentsAsserted<TComponent>()[idx];
		}

		template <typename TComponent>
			requires t_plain_component<TComponent> || t_shared_component<TComponent>
		bool tryGetComponent(const Entity entity, TComponent& outComponent)
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));

			if (!m_lookup->hasComponent(entity, typeIndex))
			{
				return false;
			}

			outComponent = getComponent<TComponent>(entity);
			return true;
		}

		//same as ComponentLookup.hasComponent()
		bool hasComponent(const Entity& entity, const std::type_index& typeIndex) const
		{
			return m_lookup->hasComponent(entity, typeIndex);
		}

		//same as ComponentLookup.hasComponent()
		template <t_component TComponent>
		bool hasComponent(const Entity& entity) const
		{
			return m_lookup->hasComponent(entity, typeid(TComponent));
		}

		template <t_component TComponent>
		bool isComponentActive(const Entity& entity) const
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(hasComponent(entity, typeIndex), "Entity %llu has no component of type %s ", entity.id(),
			         typeIndex.name());
			sizet idx = m_lookup->getComponentIndex(entity, typeIndex);
			return m_storage->getComponentsAsserted<TComponent>().isActive(idx);
		}

		template <t_component TComponent>
		void setComponentActive(const Entity& entity, const bool isActive) const
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(hasComponent(entity, typeIndex), "Entity %llu has no component of type %s ", entity.id(),
			         typeIndex.name());
			sizet idx = m_lookup->getComponentIndex(entity, typeIndex);
			m_storage->getComponentsAsserted<TComponent>().setActive(idx, isActive);
		}
	};

	class EntityEventManager
	{
		std::shared_ptr<ComponentStorage> m_storage;
		std::shared_ptr<IStructuralChangeHandler> m_structuralChangeHandler;

	public:
		EntityEventManager(std::shared_ptr<ComponentStorage> storage,
		                   std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler)
			: m_storage(std::move(storage)),
			  m_structuralChangeHandler(std::move(structuralChangeHandler))
		{
		}

		template<t_event_component T>
		void registerEvent() const
		{
			m_storage->registerEvent<T>();
		}

		template<t_event_component T>
		void createEvent(T event) const
		{
			auto& eventTable = m_storage->getEventsAsserted<T>();
			eventTable.addEvent(std::move(event));
			m_structuralChangeHandler->handleStructuralChange(typeid(T));
		}

		void rewindEvents(const std::type_index& typeIndex) const
		{
			m_storage->getRawEventTable(typeIndex).rewind();
			m_structuralChangeHandler->handleStructuralChange(typeIndex);
		}
	};

	class EntityManager
	{
		u64 m_idGen = 1;

		std::shared_ptr<ComponentStorage> m_storage;
		std::shared_ptr<ComponentLookup> m_lookup;

		std::shared_ptr<IStructuralChangeHandler> m_structuralChangeHandler;

		//for named entities
		eastl::hash_map<eastl::string, Entity, eastl::hash<eastl::string>, eastl::equal_to<eastl::string>,
		                HeapAllocator> m_namedEntities;

	public:
		EntityManager(std::shared_ptr<ComponentStorage> storage, std::shared_ptr<ComponentLookup> lookup,
		              std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler,
		              const HeapAllocator& allocator) :
			m_storage(std::move(storage)),
			m_lookup(std::move(lookup)),
			m_structuralChangeHandler(std::move(structuralChangeHandler)),
			m_namedEntities(allocator)
		{
		}

		bool isNamePresent(const cstring name)
		{
			return m_namedEntities.find(eastl::string(name)) != m_namedEntities.
				end();
		}

		bool isEntityNamed(const Entity entity)
		{
			for (auto& pair : m_namedEntities)
			{
				if (pair.second == entity)
				{
					//m_namedEntities.erase()
					return true;
				}
			}
			return false;
		}

		Entity createEntity();

		Entity createEntity(const cstring name)
		{
			Entity entity = createEntity();
			m_namedEntities.emplace(eastl::string(name), entity);
			return entity;
		}

		void setName(const Entity entity, const cstring name)
		{
			m_namedEntities[eastl::string(name)] = entity;
		}

		Entity getNamedEntity(const cstring name)
		{
			SASSERTM(isNamePresent(name), "There is no entity with name %s", eastl::string(name));
			return m_namedEntities.at(eastl::string(name));
		}

		//note: delete all components using compile time type resolve first if possible
		//(should use ComponentManager for this)
		//this func will delete all other components through runtime resolve
		//so call it last to cleanup all entity's references
		void deleteEntity(const Entity entity);

	private:
		u64 getNextId();
	};

	//only works for regular IComponents
	template <t_plain_component TComponent>
	class CommandBuffer
	{
		std::type_index m_typeIndex = std::type_index(typeid(TComponent));

		ComponentStorage* m_storage;
		ComponentLookup* m_lookup;

		ComponentTable<TComponent> m_componentsToAdd;

		eastl::vector<Entity, spite::HeapAllocator> m_entitiesToRemove;

		std::shared_ptr<IStructuralChangeHandler> m_structuralChangeHandler;

	public:
		CommandBuffer(ComponentStorage* storage, ComponentLookup* lookup, const spite::HeapAllocator& allocator,
		              std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler) :
			m_storage(storage),
			m_lookup(lookup), m_componentsToAdd(allocator), m_entitiesToRemove(allocator),
			m_structuralChangeHandler(std::move(structuralChangeHandler))
		{
		}

		void reserveForAddition(sizet capacity)
		{
			m_componentsToAdd.reserve(capacity);
		}

		void reserveForRemoval(sizet capacity)
		{
			m_entitiesToRemove.reserve(capacity);
		}

		void addComponent(const Entity entity, TComponent component, bool isActive = true)
		{
			m_componentsToAdd.addComponent(component, entity, isActive);
		}

		//removal is not bulk for now, but the difference with bulk removal might be negligable,
		//so keep it for now
		void removeComponent(const Entity entity)
		{
			m_entitiesToRemove.push_back(entity);
		}

		void commit()
		{
			sizet componentsSize = m_componentsToAdd.getOccupiedSize();
			sizet entitesSize = m_entitiesToRemove.size();
			if (componentsSize != 0)
			{
				auto& componentTable = m_storage->getComponentsAsserted<TComponent>();

				sizet topIndex = static_cast<sizet>(componentTable.getTopIndex() + 1);
				componentTable.addComponents(m_componentsToAdd);

				for (sizet i = 0; i < componentsSize; ++i, ++topIndex)
				{
					m_lookup->addComponentToLookup(m_componentsToAdd.owner(i), m_typeIndex, topIndex);
				}
			}
			//removal strongly depends on how PooledVector works
			if (entitesSize != 0)
			{
				auto& componentTable = m_storage->getComponentsAsserted<TComponent>();
				SASSERTM(componentTable.getOccupiedSize() > 0,
				         "Component table is already empty on removing using commandbuffer");
				sizet topIndex = static_cast<sizet>(componentTable.getTopIndex());

				for (sizet i = 0; i < entitesSize; ++i, --topIndex)
				{
					//set lookup indices first (top component gets the deleted component's idx) 
					Entity removedEntity = m_entitiesToRemove[i];
					sizet index = m_lookup->getComponentIndex(removedEntity, m_typeIndex);
					m_lookup->removeComponentFromLookup(removedEntity, m_typeIndex);
					Entity topEntity = componentTable.owner(topIndex);

					if (removedEntity != topEntity)
					{
						m_lookup->setComponentIndex(topEntity, m_typeIndex, index);
					}

					componentTable.removeComponent(index);
				}
			}
			m_componentsToAdd.rewind();

			m_structuralChangeHandler->handleStructuralChange(m_typeIndex);
		}
	};


	inline Entity::Entity(const u64 id): m_id(id)
	{
	}

	inline u64 Entity::id() const
	{
		return m_id;
	}

	inline bool operator==(const Entity& lhs, const Entity& rhs)
	{
		return lhs.m_id == rhs.m_id;
	}

	inline bool operator!=(const Entity& lhs, const Entity& rhs)
	{
		return !(lhs == rhs);
	}

	inline size_t Entity::hash::operator()(const spite::Entity& entity) const
	{
		return entity.id();
	}

	template <t_plain_component TComponent>
	ComponentTable<TComponent>::ComponentTable(const spite::HeapAllocator& allocator, const sizet initialSize):
		m_componentsStatuses(allocator, initialSize), m_componentsOwners(allocator, initialSize),
		m_components(allocator, initialSize)
	{
	}

	inline LookupData::LookupData(const std::type_index type,
	                              const sizet index): type(type), index(index)
	{
	}

	inline ComponentLookup::ComponentLookup(const spite::HeapAllocator& allocator): m_lookup(allocator),
		m_allocator(allocator)
	{
	}

	inline void ComponentLookup::trackEntity(const Entity entity)
	{
		SASSERTM(!isEntityTracked(entity), "Entity %llu is already tracked by component lookup instance",
		         entity.id())

		m_lookup.emplace(entity, PooledVector<LookupData>(m_allocator));
	}

	inline void ComponentLookup::untrackEntity(const Entity entity)
	{
		SASSERTM(isEntityTracked(entity), "Entity %llu is not tracked by component lookup instance",
		         entity.id())
		m_lookup.erase(entity);
	}

	inline bool ComponentLookup::isEntityTracked(const Entity entity)
	{
		auto iterator = m_lookup.find(entity);
		return iterator != m_lookup.end();
	}

	inline PooledVector<LookupData>& ComponentLookup::getLookupData(const Entity entity)
	{
		SASSERTM(isEntityTracked(entity), "Entity %llu is not tracked by component lookup instance",
		         entity.id())
		return m_lookup.at(entity);
	}

	inline bool ComponentLookup::hasComponent(const Entity entity, const std::type_index typeIndex)
	{
		SASSERTM(isEntityTracked(entity), "Entity %llu is not tracked by component lookup instance",
		         entity.id())

		auto& vec = m_lookup.at(entity);

		for (sizet i = 0, size = vec.getOccupiedSize(); i < size; ++i)
		{
			if (vec[i].type == typeIndex)
			{
				return true;
			}
		}
		return false;
	}

	inline sizet ComponentLookup::getComponentIndex(const Entity entity, const std::type_index typeIndex)
	{
		sizet lookupIndex = getComponentLookupIndex(entity, typeIndex);

		auto& vec = m_lookup.at(entity);
		return vec[lookupIndex].index;
	}

	inline void ComponentLookup::addComponentToLookup(const Entity entity, const std::type_index typeIndex,
	                                                  const sizet componentIndex)
	{
		SASSERTM(!hasComponent(entity,typeIndex), "Entity %llu already has component of type %s", entity.id(),
		         typeIndex.name())
		auto& vec = m_lookup.at(entity);
		vec.addElement(LookupData(typeIndex, componentIndex));
	}

	inline void ComponentLookup::removeComponentFromLookup(const Entity entity, const std::type_index typeIndex)
	{
		sizet lookupIndex = getComponentLookupIndex(entity, typeIndex);

		auto& vec = m_lookup.at(entity);
		vec.removeElement(lookupIndex);
	}

	inline void ComponentLookup::setComponentIndex(const Entity entity, const std::type_index typeIndex,
	                                               const sizet newIndex)
	{
		sizet lookupIndex = getComponentLookupIndex(entity, typeIndex);

		auto& vec = m_lookup.at(entity);
		vec[lookupIndex].index = newIndex;
	}


	inline sizet ComponentLookup::getComponentLookupIndex(const Entity entity, const std::type_index typeIndex)
	{
		SASSERTM(isEntityTracked(entity), "Entity %llu is not tracked by component lookup instance",
		         entity.id())

		auto& vec = m_lookup.at(entity);
		for (sizet i = 0, size = vec.getOccupiedSize(); i < size; ++i)
		{
			if (vec[i].type == typeIndex)
			{
				return i;
			}
		}
		int index = -1;

		SASSERTM(index != -1, "Component lookup data for type %s, entity %llu was not present", typeIndex.name(),
		         entity.id())
		return index;
	}

	inline void EntityManager::deleteEntity(const Entity entity)
	{
		auto& lookupData = m_lookup->getLookupData(entity);
		for (sizet i = 0, size = lookupData.getOccupiedSize(); i < size; ++i)
		{
			std::type_index type = lookupData[i].type;
			sizet index = lookupData[i].index;
			IComponentProvider& provider =
				m_storage->getRawProviderAsserted(type);

			sizet n = 0;
			Entity* topEntities = provider.getTopEntities(n);
			for (sizet j = 0; j < n; ++j)
			{
				m_lookup->setComponentIndex(topEntities[i], type, index);
			}

			bool isStructuralChange = provider.removeComponent(entity, index);

			if (isStructuralChange)
			{
				m_structuralChangeHandler->handleStructuralChange(type);
			}
		}

		m_lookup->untrackEntity(entity);

		//delete name record for named entities 
		for (auto iter = m_namedEntities.begin(), end = m_namedEntities.end(); iter != end; ++iter)
		{
			if (iter->second == entity)
			{
				m_namedEntities.erase(iter);
				return;
			}
		}
	}

	inline Entity EntityManager::createEntity()
	{
		Entity entity(getNextId());
		m_lookup->trackEntity(entity);
		return entity;
	}

	inline u64 EntityManager::getNextId()
	{
		return m_idGen++;
	}
}
