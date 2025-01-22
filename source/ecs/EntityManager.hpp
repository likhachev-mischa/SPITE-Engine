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
	typedef HeapAllocator ComponentAllocator;

	struct Entity
	{
	private:
		u64 m_id;

	public:
		explicit Entity(const u64 id);
		u64 getId() const;
		friend bool operator==(const Entity& lhs, const Entity& rhs);
		friend bool operator!=(const Entity& lhs, const Entity& rhs);

		struct hash
		{
			size_t operator()(const spite::Entity& entity) const;
		};
	};


	struct IComponent
	{
		bool isActive;
		Entity owner;

		IComponent(): isActive(false), owner(-1)
		{
		}

		IComponent(const IComponent& other) = default;
		IComponent(IComponent&& other) = default;
		IComponent& operator=(const IComponent& other) = default;
		IComponent& operator=(IComponent&& other) = default;

		virtual ~IComponent() = default;
	};

	template <typename TComponent>
	concept t_component = std::is_base_of_v<IComponent, TComponent> && std::is_default_constructible_v<TComponent>;

	//interface for component vector usage when type is resolved in runtime
	//any casting should be avoided whenever possible
	class IComponentProvider
	{
	public:
		virtual ~IComponentProvider() = default;
		virtual void removeComponent(sizet idx) = 0;
		virtual void removeComponents(sizet* array, const sizet n) = 0;
		virtual Entity getTopEntity();
	};

	template <typename T>
	class PooledVector
	{
		using VectorType = eastl::vector<T, spite::HeapAllocator>;
		VectorType m_vector;

		int m_topIdx;

	public:
		explicit PooledVector(const spite::HeapAllocator& allocator, const sizet initialSize = 10);

		PooledVector(const PooledVector& other) = delete;
		PooledVector(PooledVector&& other) = delete;
		PooledVector& operator=(const PooledVector& other) = delete;
		PooledVector& operator=(PooledVector&& other) = delete;
		virtual ~PooledVector() = default;

		//just moves top pointer: no reallocations, but the table is free
		void rewind();

		sizet getTopIndex() const;

		sizet getOccupiedSize() const;

		sizet getTotalSize();

		sizet getCapacity();

		//occupied slots / all slots
		float getFillFactor();

		void setCapacity(sizet n);

		//preferable to use this
		template <typename InputIterator>
		void addElements(const InputIterator& begin, const InputIterator& end);

		//avoid this, use bulk insertion whenever possible
		void addElement(const T& element);

		/**
		 * \brief simply replaces elements on input indices with elements 
		 * from the top of the table and decreases top pointer
		 * \param array : an array of indices
		 * WARNING: all indices must be unique - undefined behaviour otherwise
		 * \param n : array size
		 */
		void removeElements(const sizet* array, const sizet n);

		void removeElement(sizet idx);

		class iterator
		{
			typename VectorType::iterator m_current;
			typename VectorType::iterator m_end;

		public:
			iterator(typename VectorType::iterator current, typename VectorType::iterator end);
			iterator& operator++();
			T& operator*() const;
			bool operator!=(const iterator& other) const;
		};

		T& operator[](sizet n);
		iterator begin();
		iterator end();
	};

	template <t_component TComponent>
	class ComponentTable final : public PooledVector<TComponent>, IComponentProvider
	{
	public:
		ComponentTable(const spite::HeapAllocator& allocator, const sizet initialSize = 10);

		ComponentTable(const ComponentTable& other) = delete;
		ComponentTable(ComponentTable&& other) = delete;
		ComponentTable& operator=(const ComponentTable& other) = delete;
		ComponentTable& operator=(ComponentTable&& other) = delete;

		void removeComponent(sizet idx) override;

		void removeComponents(sizet* array, const sizet n) override;

		Entity getTopEntity() override;

		~ComponentTable() override = default;
	};


	class ComponentStorage
	{
		eastl::hash_map<std::type_index, IComponentProvider*, std::hash<std::type_index>, eastl::equal_to<
			                std::type_index>,
		                ComponentAllocator> m_storage;
		ComponentAllocator m_componentAllocator;

	public:
		explicit ComponentStorage(const ComponentAllocator& componentAllocator);

		template <t_component TComponent>
		void registerComponent();

		bool isComponentRegistred(const std::type_index typeIndex);

		//used only for entity deletion
		//should avoid using it otherwise
		IComponentProvider& getRawProvider(const std::type_index typeIndex);

		template <t_component TComponent>
		ComponentTable<TComponent>& getComponentsAsserted();

		template <t_component TComponent>
		ComponentTable<TComponent>& getComponentsSafe();
	};

	class ComponentLookup
	{
		struct LookupData;
		eastl::hash_map<Entity, PooledVector<ComponentLookup::LookupData>, Entity::hash, eastl::equal_to<
			                Entity>, spite::HeapAllocator> m_lookup;
		spite::HeapAllocator m_allocator;

	public:
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

		ComponentLookup(const spite::HeapAllocator& allocator);

		void trackEntity(const Entity entity);

		void untrackEntity(const Entity entity);

		bool isEntityTracked(const Entity entity);

		PooledVector<LookupData>& getLookupData(const Entity entity);

		bool hasComponent(const Entity entity, const std::type_index typeIndex);

		sizet getComponentIndex(const Entity entity, const std::type_index typeIndex);

		void addComponent(const Entity entity, const std::type_index typeIndex, const sizet componentIndex);

		void removeComponent(const Entity entity, const std::type_index typeIndex);

		void setComponentIndex(const Entity entity, const std::type_index typeIndex, const sizet newIndex);

	private:
		sizet getComponentLookupIndex(const Entity entity, const std::type_index typeIndex);
	};

	class ComponentManager
	{
		std::shared_ptr<ComponentStorage> m_storage;
		std::shared_ptr<ComponentLookup> m_lookup;

	public:
		ComponentManager(std::shared_ptr<ComponentStorage> componentStorage,
		                 std::shared_ptr<ComponentLookup> componentLookup);

		//note: any changes to passed component param won't be saved in storage after this func
		template <t_component TComponent>
		void addComponent(Entity entity, TComponent& component);

		template <t_component TComponent>
		void addComponent(Entity entity);

		template <t_component TComponent>
		void removeComponent(const Entity entity);
	};

	class EntityManager
	{
		u64 m_idGen = 1;

		std::shared_ptr<ComponentStorage> m_storage;
		std::shared_ptr<ComponentLookup> m_lookup;

	public:
		Entity createEntity();

		//note: delete all components using compile time type resolve first if possible
		//(should use ComponentManager for this)
		//this func will delete all other components through runtime resolve
		//so call it last to cleanup all entity's references
		void deleteEntity(const Entity entity) const;

	private:
		u64 getNextId();
	};

	template <t_component TComponent>
	class CommandBuffer
	{
		std::type_index m_typeIndex = std::type_index(typeid(TComponent));

		ComponentStorage* m_storage;
		ComponentLookup* m_lookup;

		eastl::vector<TComponent, spite::HeapAllocator> m_components;

		eastl::vector<Entity, spite::HeapAllocator> m_entities;

	public:
		CommandBuffer(ComponentStorage* storage, ComponentLookup* lookup);

		void reserveForAddition(sizet capacity);

		void reserveForRemoval(sizet capacity);

		void addComponent(const Entity entity, TComponent& component);

		void removeComponent(const Entity entity);

		//removal is not bulk for now, but the difference with bulk removal might be negligable,
		//so keep it for now
		void commit();
	};

	template <t_component TComponent>
	class QueryFilter
	{
		using VectorType = eastl::vector<sizet, spite::HeapAllocator>;
		VectorType m_indices;
		ComponentLookup* m_lookup;
		PooledVector<TComponent>& m_table;

		eastl::vector<std::type_index, spite::HeapAllocator> m_dependencies;

	public:
		QueryFilter(ComponentLookup* lookup, ComponentStorage* storage, const spite::HeapAllocator& allocator);

		bool isDependentOn(const std::type_index typeIndex);

		sizet getSize() const;

		sizet getComponentIndex(sizet filterIndex);

		TComponent& operator[](sizet n);

		//checks for active and inactive components
		//temporary allocates stuff,so the filter needs to be cached upon system creation
		QueryFilter& hasComponent(const std::type_index typeIndex);

		//checks for active and inactive components
		//temporary allocates stuff,so the filter needs to be cached upon system creation
		QueryFilter& hasNoComponent(const std::type_index typeIndex);
	};

	inline Entity::Entity(const u64 id): m_id(id)
	{
	}

	inline u64 Entity::getId() const
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
		return entity.getId();
	}


	template <typename T>
	PooledVector<T>::PooledVector(const spite::HeapAllocator& allocator, const sizet initialSize):
		m_vector(allocator)
	{
		m_vector.reserve(initialSize);
		m_topIdx = -1;
	}

	template <typename T>
	void PooledVector<T>::rewind()
	{
		m_topIdx = -1;
	}

	template <typename T>
	sizet PooledVector<T>::getTopIndex() const
	{
		SASSERTM(m_topIdx > -1, "Vector is uninitialized");
		return m_topIdx;
	}

	template <typename T>
	sizet PooledVector<T>::getOccupiedSize() const
	{
		SASSERTM(m_topIdx >= -1, "Vector is uninitialized");
		return static_cast<sizet>(m_topIdx + 1);
	}

	template <typename T>
	sizet PooledVector<T>::getTotalSize()
	{
		return m_vector.size();
	}

	template <typename T>
	sizet PooledVector<T>::getCapacity()
	{
		return m_vector.capacity();
	}

	template <typename T>
	float PooledVector<T>::getFillFactor()
	{
		return getOccupiedSize() / m_vector.capacity();
	}

	template <typename T>
	void PooledVector<T>::setCapacity(sizet n)
	{
		SASSERTM(n < m_topIdx + 1, "SET CAPACITY WILL DELETE STORED ELEMENTS");
		m_vector.set_capacity(n);
	}

	template <typename T>
	template <typename InputIterator>
	void PooledVector<T>::addElements(const InputIterator& begin, const InputIterator& end)
	{
		auto destIter = m_vector.begin() + m_topIdx + 1;
		auto srcIter = begin;

		for (sizet i = m_topIdx + 1, size = m_vector.size(); i < size && srcIter != end; ++i, ++destIter, ++
		     srcIter, ++m_topIdx)
		{
			*destIter = *srcIter;
			++destIter;
			++srcIter;
		}
		if (srcIter != end)
		{
			sizet resizeCount = std::distance(srcIter, end);;
			SDEBUG_LOG("pooled vector of type %s: insert %llu elems, fill factor: %f, total size: %llu",
			           std::type_index(typeid(T)).name(), resizeCount,
			           getFillFactor(), m_vector.size());
			m_vector.insert(destIter, srcIter, end);
			m_topIdx += resizeCount;
		}
	}

	template <typename T>
	void PooledVector<T>::addElement(const T& element)
	{
		++m_topIdx;
		if (m_topIdx < m_vector.size())
		{
			m_vector[m_topIdx] = element;
		}
		else
		{
			m_vector.push_back(element);
		}
	}

	template <typename T>
	void PooledVector<T>::removeElements(const sizet* array, const sizet n)
	{
		SASSERTM(m_topIdx >= n-1, "TRYING TO REMOVE MORE elements THAN PRESENT");
		for (sizet i = 0; i < n; ++i)
		{
			sizet idx = array[i];
			m_vector[idx] = m_vector[m_topIdx];
			--m_topIdx;
		}
	}

	template <typename T>
	void PooledVector<T>::removeElement(sizet idx)
	{
		SASSERTM(m_topIdx >= 0, "TRYING TO REMOVE ELEMENT IN EMPTY VECTOR");
		m_vector[idx] = m_vector[m_topIdx];
		--m_topIdx;
	}

	template <typename T>
	PooledVector<
		T>::iterator::iterator(typename VectorType::iterator current,
		                       typename VectorType::iterator end): m_current(current), m_end(end)
	{
	}

	template <typename T>
	typename PooledVector<T>::iterator& PooledVector<T>::iterator::operator++()
	{
		++m_current;
		return *this;
	}

	template <typename T>
	T& PooledVector<T>::iterator::operator*() const
	{
		return *m_current;
	}

	template <typename T>
	bool PooledVector<T>::iterator::operator!=(const iterator& other) const
	{
		return m_current != other.m_current;
	}

	template <typename T>
	T& PooledVector<T>::operator[](sizet n)
	{
		return m_vector[n];
	}

	template <typename T>
	typename PooledVector<T>::iterator PooledVector<T>::begin()
	{
		return iterator(m_vector.begin(), m_vector.end());
	}

	template <typename T>
	typename PooledVector<T>::iterator PooledVector<T>::end()
	{
		return iterator(m_vector.end(), m_vector.end());
	}

	template <t_component TComponent>
	ComponentTable<TComponent>::ComponentTable(const spite::HeapAllocator& allocator, const sizet initialSize): PooledVector(
		allocator, initialSize)
	{
	}

	template <t_component TComponent>
	void ComponentTable<TComponent>::removeComponent(sizet idx)
	{
		this->removeElement(idx);
	}

	template <t_component TComponent>
	void ComponentTable<TComponent>::removeComponents(sizet* array, const sizet n)
	{
		this->removeElements(array, n);
	}

	template <t_component TComponent>
	Entity ComponentTable<TComponent>::getTopEntity()
	{
		return this->operator[](this->getTopIndex()).owner;
	}

	inline ComponentStorage::ComponentStorage(const ComponentAllocator& componentAllocator):
		m_storage(componentAllocator),
		m_componentAllocator(
			componentAllocator)
	{
	}

	template <t_component TComponent>
	void ComponentStorage::registerComponent()
	{
		std::type_index typeIndex = std::type_index(typeid(TComponent));
		SASSERTM(!isComponentRegistred(typeIndex), "Component of type %s is already registered", typeIndex.name());

		m_storage.emplace(typeIndex,
		                  new ComponentTable<TComponent>(m_componentAllocator));
	}

	inline bool ComponentStorage::isComponentRegistred(const std::type_index typeIndex)
	{
		auto iterator = m_storage.find(typeIndex);
		return iterator != m_storage.end();
	}

	inline IComponentProvider& ComponentStorage::getRawProvider(const std::type_index typeIndex)
	{
		SASSERTM(isComponentRegistred(typeIndex), "No components of type %s exist in storage", typeIndex.name());
		return *m_storage.at(typeIndex);
	}


	template <t_component TComponent>
	ComponentTable<TComponent>& ComponentStorage::getComponentsAsserted()
	{
		std::type_index typeIndex = std::type_index(typeid(TComponent));
		SASSERTM(isComponentRegistred(typeIndex), "No components of type %s exist in storage", typeIndex.name());

		auto& value = m_storage.at(typeIndex);
		auto table = static_cast<ComponentTable<TComponent>*>(value);
		return *table;
	}

	template <t_component TComponent>
	ComponentTable<TComponent>& ComponentStorage::getComponentsSafe()
	{
		std::type_index typeIndex = std::type_index(typeid(TComponent));
		if (!isComponentRegistred(typeIndex))
		{
			registerComponent<TComponent>();
		}
		auto& value = m_storage.at(typeIndex);
		auto table = static_cast<ComponentTable<TComponent>*>(value);
		return *table;
	}

	inline ComponentLookup::LookupData::LookupData(const std::type_index type,
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
		         entity.getId())

		m_lookup.emplace(entity, PooledVector<LookupData>(m_allocator));
	}

	inline void ComponentLookup::untrackEntity(const Entity entity)
	{
		SASSERTM(isEntityTracked(entity), "Entity %llu is not tracked by component lookup instance",
		         entity.getId());
		m_lookup.erase(entity);
	}

	inline bool ComponentLookup::isEntityTracked(const Entity entity)
	{
		auto iterator = m_lookup.find(entity);
		return iterator != m_lookup.end();
	}

	inline PooledVector<ComponentLookup::LookupData>& ComponentLookup::getLookupData(const Entity entity)
	{
		SASSERTM(isEntityTracked(entity), "Entity %llu is not tracked by component lookup instance",
		         entity.getId());
		return m_lookup.at(entity);
	}

	inline bool ComponentLookup::hasComponent(const Entity entity, const std::type_index typeIndex)
	{
		SASSERTM(isEntityTracked(entity), "Entity %llu is not tracked by component lookup instance",
		         entity.getId());

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

	inline void ComponentLookup::addComponent(const Entity entity, const std::type_index typeIndex,
	                                          const sizet componentIndex)
	{
		SASSERTM(!hasComponent(entity,typeIndex), "Entity %llu already has component of type %s", entity.getId(),
		         typeIndex.name());
		auto& vec = m_lookup.at(entity);
		vec.addElement(LookupData(typeIndex, componentIndex));
	}

	inline void ComponentLookup::removeComponent(const Entity entity, const std::type_index typeIndex)
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
		         entity.getId());

		auto& vec = m_lookup.at(entity);
		for (sizet i = 0, size = vec.getOccupiedSize(); i < size; ++i)
		{
			if (vec[i].type == typeIndex)
			{
				return i;
			}
		}
		sizet index = -1;

		SASSERTM(index != -1, "Component lookup data for type %s, entity %llu was not present", typeIndex.name(),
		         entity.getId())
		return index;
	}

	inline ComponentManager::ComponentManager(std::shared_ptr<ComponentStorage> componentStorage,
	                                          std::shared_ptr<ComponentLookup> componentLookup):
		m_storage(std::move(componentStorage)),
		m_lookup(std::move(componentLookup))
	{
	}

	inline Entity EntityManager::createEntity()
	{
		Entity entity(getNextId());
		m_lookup->trackEntity(entity);
		return entity;
	}

	inline void EntityManager::deleteEntity(const Entity entity) const
	{
		auto& lookupData = m_lookup->getLookupData(entity);
		for (sizet i = 0, size = lookupData.getOccupiedSize(); i < size; ++i)
		{
			std::type_index type = lookupData[i].type;
			sizet index = lookupData[i].index;
			IComponentProvider& provider =
				m_storage->getRawProvider(type);
			Entity topEntity = provider.getTopEntity();
			m_lookup->setComponentIndex(topEntity, type, index);
			provider.removeComponent(index);
		}

		m_lookup->untrackEntity(entity);
	}

	inline u64 EntityManager::getNextId()
	{
		return m_idGen++;
	}

	template <t_component TComponent>
	void ComponentManager::addComponent(Entity entity, TComponent& component)
	{
		component.owner = entity;

		auto& components = m_storage->getComponentsSafe<TComponent>();
		components.addElement(component);
		sizet componentIndex = components.getTopIndex();

		m_lookup->addComponent(entity, std::type_index(typeid(TComponent)), componentIndex);
	}

	template <t_component TComponent>
	void ComponentManager::addComponent(Entity entity)
	{
		TComponent component;
		component.owner = entity;

		auto& components = m_storage->getComponentsSafe<TComponent>();
		components.addElement(component);
		sizet componentIndex = components.getTopIndex();

		m_lookup->addComponent(entity, std::type_index(typeid(TComponent)), componentIndex);
	}

	template <t_component TComponent>
	void ComponentManager::removeComponent(const Entity entity)
	{
		auto& componentTable = m_storage->getComponentsAsserted<TComponent>();
		sizet topIndex = componentTable.getTopIndex();

		std::type_index typeIndex = std::type_index(typeid(TComponent));

		sizet index = m_lookup->getComponentIndex(entity, typeIndex);
		m_lookup->removeComponent(entity, typeIndex);
		Entity topEntity = componentTable[topIndex].owner;
		m_lookup->setComponentIndex(topEntity, typeIndex, index);

		componentTable.removeElement(index);
	}

	template <t_component TComponent>
	CommandBuffer<
		TComponent>::CommandBuffer(ComponentStorage* storage, ComponentLookup* lookup): m_storage(storage),
		m_lookup(lookup)
	{
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::reserveForAddition(sizet capacity)
	{
		m_components.reserve(capacity);
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::reserveForRemoval(sizet capacity)
	{
		m_entities.reserve(capacity);
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::addComponent(const Entity entity, TComponent& component)
	{
		component.owner = entity;
		m_components.push_back(component);
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::removeComponent(const Entity entity)
	{
		m_entities.push_back(entity);
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::commit()
	{
		sizet componentsSize = m_components.size();
		sizet entitesSize = m_entities.size();
		if (componentsSize != 0)
		{
			auto& componentTable = m_storage->getComponentsAsserted<TComponent>();
			sizet topIndex = componentTable.getTopIndex();
			componentTable.addElements(m_components.begin(), m_components.end());

			for (sizet i = 0; i < componentsSize; ++i, ++topIndex)
			{
				m_lookup->addComponent(m_components[i].owner, m_typeIndex, topIndex);
			}
		}
		//removal strongly depends on how PooledVector works
		if (entitesSize != 0)
		{
			auto& componentTable = m_storage->getComponentsAsserted<TComponent>();
			sizet topIndex = componentTable.getTopIndex();

			for (sizet i = 0; i < entitesSize; ++i, --topIndex)
			{
				//set lookup indices first (top component gets the deleted component's idx) 
				sizet index = m_lookup->getComponentIndex(m_entities[i], m_typeIndex);
				m_lookup->removeComponent(m_entities[i], m_typeIndex);
				Entity topEntity = componentTable[topIndex].owner;
				m_lookup->setComponentIndex(topEntity, m_typeIndex, index);

				componentTable.removeElement(index);
			}
		}
	}

	template <t_component TComponent>
	QueryFilter<TComponent>::QueryFilter(ComponentLookup* lookup, ComponentStorage* storage,
	                                     const spite::HeapAllocator& allocator): m_indices(allocator), m_lookup(lookup)
	{
		m_table = storage->getComponentsAsserted<TComponent>();
		sizet size = m_table.getOccupiedSize();
		m_indices.reserve(size);

		for (sizet i = 0; i < size; ++i)
		{
			m_indices.push_back(i);
		}
	}

	template <t_component TComponent>
	bool QueryFilter<TComponent>::isDependentOn(const std::type_index typeIndex)
	{
		for (const auto dependency : m_dependencies)
		{
			if (dependency == typeIndex)
			{
				return true;
			}
		}

		return false;
	}

	template <t_component TComponent>
	sizet QueryFilter<TComponent>::getSize() const
	{
		return m_indices.size();
	}

	template <t_component TComponent>
	sizet QueryFilter<TComponent>::getComponentIndex(sizet filterIndex)
	{
		return m_indices[filterIndex];
	}

	template <t_component TComponent>
	TComponent& QueryFilter<TComponent>::operator[](sizet n)
	{
		return m_table[m_indices[n]];
	}

	template <t_component TComponent>
	QueryFilter<TComponent>& QueryFilter<TComponent>::hasComponent(const std::type_index typeIndex)
	{
		m_dependencies.push_back(typeIndex);

		VectorType prevIndices(m_indices, m_indices.get_allocator());
		m_indices.clear();

		for (const sizet& idx : prevIndices)
		{
			Entity owner = m_table[idx].owner;

			if (m_lookup->hasComponent(owner, typeIndex))
			{
				m_indices.push_back(idx);
			}
		}
		return *this;
	}

	template <t_component TComponent>
	QueryFilter<TComponent>& QueryFilter<TComponent>::hasNoComponent(const std::type_index typeIndex)
	{
		m_dependencies.push_back(typeIndex);

		VectorType prevIndices(m_indices, m_indices.get_allocator());
		m_indices.clear();

		for (const sizet& idx : prevIndices)
		{
			Entity owner = m_table[idx].owner;
			if (!m_lookup->hasComponent(owner, typeIndex))
			{
				m_indices.push_back(idx);
			}
		}
		return *this;
	}
}
