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
		bool isActive;
		Entity owner;

		IComponent(): isActive(false), owner(-1)
		{
		}

		IComponent(const IComponent& other): isActive(other.isActive), owner(other.owner)
		{
		}

		IComponent(IComponent&& other) noexcept: isActive(other.isActive), owner(other.owner)
		{
		}

		IComponent& operator=(const IComponent& other)
		{
			if (this == &other)
				return *this;

			isActive = other.isActive;
			owner = other.owner;
			return *this;
		}

		IComponent& operator=(IComponent&& other) noexcept
		{
			if (this == &other)
				return *this;

			isActive = other.isActive;
			owner = other.owner;
			return *this;
		}

		virtual ~IComponent() = default;
	};

	template <typename TComponent>
	concept t_component = std::is_base_of_v<IComponent, TComponent> && std::is_default_constructible_v<TComponent> &&
		std::is_copy_constructible_v<TComponent> &&
		std::is_move_assignable_v<TComponent> && std::is_move_constructible_v<TComponent>;

	//interface for component vector usage when type is resolved in runtime
	//any casting should be avoided whenever possible
	class IComponentProvider
	{
	public:
		virtual void removeComponent(sizet idx) = 0;
		virtual void removeComponents(sizet* array, const sizet n) = 0;
		virtual bool isEmpty() = 0;
		virtual Entity getTopEntity() = 0;
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

		void setCapacity(sizet n)
		{
			SASSERTM(n < m_topIdx + 1, "SET CAPACITY WILL DELETE STORED ELEMENTS");
			m_vector.set_capacity(n);
		}

		//preferable to use this
		//WARNING: uses move assignment/insertion for passed params
		template <typename InputIterator>
		void addElements(const InputIterator& begin, const InputIterator& end)
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
				SDEBUG_LOG("pooled vector of type %s: insert %llu elems, fill factor: %f, total size: %llu",
				           std::type_index(typeid(T)).name(), resizeCount,
				           getFillFactor(), m_vector.size());
				m_vector.insert(destIter, eastl::make_move_iterator(srcIter), eastl::make_move_iterator(end));
				m_topIdx += static_cast<int>(resizeCount);
			}
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

		class iterator
		{
			typename VectorType::iterator m_current;
			typename VectorType::iterator m_end;

		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			iterator(typename VectorType::iterator current, typename VectorType::iterator end) : m_current(current),
				m_end(end)
			{
			}

			iterator& operator++()
			{
				++m_current;
				return *this;
			}

			reference operator*() const
			{
				return *m_current;
			}

			bool operator!=(const iterator& other) const
			{
				return m_current != other.m_current;
			}
		};

		T& operator[](sizet n)
		{
			return m_vector[n];
		}

		iterator begin()
		{
			return iterator(m_vector.begin(), m_vector.end());
		}

		iterator end()
		{
			return iterator(m_vector.end(), m_vector.end());
		}
	};

	template <t_component TComponent>
	class ComponentTable final : public PooledVector<TComponent>, public IComponentProvider
	{
	public:
		explicit ComponentTable(const spite::HeapAllocator& allocator, const sizet initialSize = 10);

		ComponentTable(const ComponentTable& other) = delete;
		ComponentTable(ComponentTable&& other) = delete;
		ComponentTable& operator=(ComponentTable&& other) = delete;
		ComponentTable& operator=(const ComponentTable& other) = delete;

		void removeComponent(sizet idx) override
		{
			this->removeElement(idx);
		}

		void removeComponents(sizet* array, const sizet n) override
		{
			this->removeElements(array, n);
		}

		bool isEmpty() override
		{
			return this->getOccupiedSize() == 0;
		}

		Entity getTopEntity() override
		{
			return this->operator[](this->getTopIndex()).owner;
		}

		~ComponentTable() override = default;
	};


	class ComponentStorage
	{
		eastl::hash_map<std::type_index, IComponentProvider*, std::hash<std::type_index>, eastl::equal_to<
			                std::type_index>, ComponentAllocator> m_storage;

		ComponentAllocator m_componentAllocator;

	public:
		explicit ComponentStorage(const ComponentAllocator& componentAllocator) :
			m_storage(componentAllocator),
			m_componentAllocator(componentAllocator)
		{
		}

		template <t_component TComponent>
		void registerComponent()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(!isComponentRegistred(typeIndex), "Component of type %s is already registered", typeIndex.name());

			IComponentProvider* provider = new ComponentTable<TComponent>(m_componentAllocator);
			m_storage.emplace(typeIndex, provider);
		}

		bool isComponentRegistred(const std::type_index typeIndex)
		{
			auto iterator = m_storage.find(typeIndex);
			return iterator != m_storage.end();
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

		template <t_component TComponent>
		ComponentTable<TComponent>& getComponentsAsserted()
		{
			std::type_index typeIndex = std::type_index(typeid(TComponent));
			SASSERTM(isComponentRegistred(typeIndex), "No components of type %s exist in storage", typeIndex.name());

			auto& value = m_storage.at(typeIndex);
			auto table = reinterpret_cast<ComponentTable<TComponent>*>(value);
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

		~ComponentStorage()
		{
			for (const auto& pair : m_storage)
			{
				delete pair.second;
			}
			m_storage.clear(true);
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
		                 std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler);

		//note: any changes to passed component param won't be saved in storage after this func
		template <t_component TComponent>
		void addComponent(Entity entity, TComponent& component);

		template <t_component TComponent>
		void addComponent(Entity entity);

		template <t_component TComponent>
		void removeComponent(const Entity entity);

		//asserts that entity has component
		template <t_component TComponent>
		TComponent& getComponent(const Entity entity);

		template <t_component TComponent>
		bool tryGetComponent(const Entity entity, TComponent& outComponent);

		//same as ComponentLookup.hasComponent()
		bool hasComponent(const Entity& entity, const std::type_index& typeIndex) const;

		//same as ComponentLookup.hasComponent()
		template <t_component TComponent>
		bool hasComponent(const Entity& entity) const;
	};

	class EntityManager
	{
		u64 m_idGen = 1;

		std::shared_ptr<ComponentStorage> m_storage;
		std::shared_ptr<ComponentLookup> m_lookup;

		std::shared_ptr<IStructuralChangeHandler> m_structuralChangeHandler;

	public:
		EntityManager(std::shared_ptr<ComponentStorage> storage, std::shared_ptr<ComponentLookup> lookup,
		              std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler);

		Entity createEntity();

		//note: delete all components using compile time type resolve first if possible
		//(should use ComponentManager for this)
		//this func will delete all other components through runtime resolve
		//so call it last to cleanup all entity's references
		void deleteEntity(const Entity entity);

	private:
		u64 getNextId();
	};

	template <t_component TComponent>
	class CommandBuffer
	{
		std::type_index m_typeIndex = std::type_index(typeid(TComponent));

		ComponentStorage* m_storage;
		ComponentLookup* m_lookup;

		eastl::vector<TComponent, spite::HeapAllocator> m_componentsToAdd;

		eastl::vector<Entity, spite::HeapAllocator> m_entitiesToRemove;

		std::shared_ptr<IStructuralChangeHandler> m_structuralChangeHandler;

	public:
		CommandBuffer(ComponentStorage* storage, ComponentLookup* lookup, const spite::HeapAllocator& allocator,
		              std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler);

		void reserveForAddition(sizet capacity);

		void reserveForRemoval(sizet capacity);

		void addComponent(const Entity entity, TComponent& component);

		//removal is not bulk for now, but the difference with bulk removal might be negligable,
		//so keep it for now
		void removeComponent(const Entity entity);

		void commit();
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

	template <t_component TComponent>
	ComponentTable<TComponent>::ComponentTable(const spite::HeapAllocator& allocator, const sizet initialSize):
		PooledVector<TComponent>(allocator, initialSize)
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

	inline ComponentManager::ComponentManager(std::shared_ptr<ComponentStorage> componentStorage,
	                                          std::shared_ptr<ComponentLookup> componentLookup,
	                                          std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler):
		m_storage(std::move(componentStorage)),
		m_lookup(std::move(componentLookup)),
		m_structuralChangeHandler(std::move(structuralChangeHandler))
	{
	}

	inline bool ComponentManager::hasComponent(const Entity& entity, const std::type_index& typeIndex) const
	{
		return m_lookup->hasComponent(entity, typeIndex);
	}

	template <t_component TComponent>
	void ComponentManager::addComponent(Entity entity, TComponent& component)
	{
		component.owner = entity;

		auto& components = m_storage->getComponentsSafe<TComponent>();
		components.addElement(component);
		sizet componentIndex = components.getTopIndex();

		std::type_index typeIndex = std::type_index(typeid(TComponent));
		m_lookup->addComponentToLookup(entity, typeIndex, componentIndex);

		m_structuralChangeHandler->handleStructuralChange(typeIndex);
	}

	template <t_component TComponent>
	void ComponentManager::addComponent(Entity entity)
	{
		TComponent component;
		component.owner = entity;

		auto& components = m_storage->getComponentsSafe<TComponent>();
		components.addElement(component);
		sizet componentIndex = components.getTopIndex();

		std::type_index typeIndex = std::type_index(typeid(TComponent));
		m_lookup->addComponentToLookup(entity, typeIndex, componentIndex);

		m_structuralChangeHandler->handleStructuralChange(typeIndex);
	}

	template <t_component TComponent>
	void ComponentManager::removeComponent(const Entity entity)
	{
		auto& componentTable = m_storage->getComponentsAsserted<TComponent>();
		sizet topIndex = componentTable.getTopIndex();

		std::type_index typeIndex = std::type_index(typeid(TComponent));

		sizet index = m_lookup->getComponentIndex(entity, typeIndex);
		m_lookup->removeComponentFromLookup(entity, typeIndex);
		Entity topEntity = componentTable[topIndex].owner;
		if (topEntity != entity)
		{
			m_lookup->setComponentIndex(topEntity, typeIndex, index);
		}

		componentTable.removeElement(index);

		m_structuralChangeHandler->handleStructuralChange(typeIndex);
	}

	template <t_component TComponent>
	TComponent& ComponentManager::getComponent(const Entity entity)
	{
		sizet idx = m_lookup->getComponentIndex(entity, std::type_index(typeid(TComponent)));
		return m_storage->getComponentsAsserted<TComponent>()[idx];
	}

	template <t_component TComponent>
	bool ComponentManager::tryGetComponent(const Entity entity, TComponent& outComponent)
	{
		std::type_index typeIndex = std::type_index(typeid(TComponent));

		if (!m_lookup->hasComponent(entity, typeIndex))
		{
			return false;
		}

		outComponent = getComponent<TComponent>(entity);
		return true;
	}

	template <t_component TComponent>
	bool ComponentManager::hasComponent(const Entity& entity) const
	{
		return m_lookup->hasComponent(entity, typeid(TComponent));
	}

	inline EntityManager::EntityManager(std::shared_ptr<ComponentStorage> storage,
	                                    std::shared_ptr<ComponentLookup> lookup,
	                                    std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler):
		m_storage(std::move(storage)),
		m_lookup(std::move(lookup)),
		m_structuralChangeHandler(std::move(structuralChangeHandler))
	{
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
			Entity topEntity = provider.getTopEntity();
			m_lookup->setComponentIndex(topEntity, type, index);
			provider.removeComponent(index);

			m_structuralChangeHandler->handleStructuralChange(type);
		}

		m_lookup->untrackEntity(entity);
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

	template <t_component TComponent>
	CommandBuffer<TComponent>::CommandBuffer(ComponentStorage* storage, ComponentLookup* lookup,
	                                         const spite::HeapAllocator& allocator,
	                                         std::shared_ptr<IStructuralChangeHandler> structuralChangeHandler):
		m_storage(storage),
		m_lookup(lookup), m_componentsToAdd(allocator), m_entitiesToRemove(allocator),
		m_structuralChangeHandler(std::move(structuralChangeHandler))
	{
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::commit()
	{
		sizet componentsSize = m_componentsToAdd.size();
		sizet entitesSize = m_entitiesToRemove.size();
		if (componentsSize != 0)
		{
			auto& componentTable = m_storage->getComponentsAsserted<TComponent>();

			sizet topIndex = static_cast<sizet>(componentTable.getTopIndex() + 1);
			componentTable.addElements(m_componentsToAdd.begin(), m_componentsToAdd.end());

			for (sizet i = 0; i < componentsSize; ++i, ++topIndex)
			{
				m_lookup->addComponentToLookup(m_componentsToAdd[i].owner, m_typeIndex, topIndex);
			}
		}
		//removal strongly depends on how PooledVector works
		if (entitesSize != 0)
		{
			auto& componentTable = m_storage->getComponentsAsserted<TComponent>();

			SASSERTM(componentTable.getOccupiedSize() > 0, "Component table is already empty on removing using commandbuffer");
			sizet topIndex = static_cast<sizet>(componentTable.getTopIndex());

			for (sizet i = 0; i < entitesSize; ++i, --topIndex)
			{
				//set lookup indices first (top component gets the deleted component's idx) 
				Entity removedEntity = m_entitiesToRemove[i];
				sizet index = m_lookup->getComponentIndex(removedEntity, m_typeIndex);
				m_lookup->removeComponentFromLookup(removedEntity, m_typeIndex);
				Entity topEntity = componentTable[topIndex].owner;

				if (removedEntity != topEntity)
				{
					m_lookup->setComponentIndex(topEntity, m_typeIndex, index);
				}

				componentTable.removeElement(index);
			}
		}
		m_structuralChangeHandler->handleStructuralChange(m_typeIndex);
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::reserveForAddition(sizet capacity)
	{
		m_componentsToAdd.reserve(capacity);
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::reserveForRemoval(sizet capacity)
	{
		m_entitiesToRemove.reserve(capacity);
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::addComponent(const Entity entity, TComponent& component)
	{
		component.owner = entity;
		m_componentsToAdd.push_back(component);
	}

	template <t_component TComponent>
	void CommandBuffer<TComponent>::removeComponent(const Entity entity)
	{
		m_entitiesToRemove.push_back(entity);
	}
}
