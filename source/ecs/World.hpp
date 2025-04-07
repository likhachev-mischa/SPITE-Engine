#pragma once

#include <EASTL/vector_set.h>

#include "ecs/Core.hpp"
#include "ecs/Queries.hpp"

namespace spite
{
	class EntityWorld;

	//tracks component addition/removal
	//checks if component table is empty on each structural change
	class StructuralChangeTracker
	{
		std::shared_ptr<ComponentStorage> m_storage;

		eastl::vector_set<std::type_index, std::less<std::type_index>, HeapAllocator> m_emptyTables;
		eastl::vector_set<std::type_index, std::less<std::type_index>, HeapAllocator> m_nonEmptyTables;

	public:
		StructuralChangeTracker(std::shared_ptr<ComponentStorage> storage, const HeapAllocator& allocator) :
			m_storage(std::move(storage)), m_emptyTables(allocator), m_nonEmptyTables(allocator)
		{
		}

		void reset()
		{
			m_emptyTables.clear();
			m_nonEmptyTables.clear();
		}

		void recordChange(const std::type_index typeIndex)
		{
			bool isComponentPresent = m_storage->hasAnyComponents(typeIndex);

			if (isComponentPresent)
			{
				m_emptyTables.erase(typeIndex);
				m_nonEmptyTables.insert(typeIndex);
			}
			else
			{
				m_nonEmptyTables.erase(typeIndex);
				m_emptyTables.insert(typeIndex);
			}
		}

		eastl::vector_set<std::type_index, std::less<std::type_index>, spite::HeapAllocator>& getEmptyTables()
		{
			return m_emptyTables;
		}

		eastl::vector_set<std::type_index, std::less<std::type_index>, spite::HeapAllocator>& getNonEmptyTables()
		{
			return m_nonEmptyTables;
		}
	};

	class StructuralChangeHandler : public IStructuralChangeHandler
	{
		std::shared_ptr<QueryBuilder> m_queryBuilder;
		std::shared_ptr<StructuralChangeTracker> m_tracker;

	public:
		StructuralChangeHandler(std::shared_ptr<QueryBuilder> queryBuilder,
			std::shared_ptr<StructuralChangeTracker> tracker)
			: m_queryBuilder(std::move(queryBuilder)), m_tracker(std::move(tracker))
		{
		}

		void handleStructuralChange(const std::type_index& typeIndex) override
		{
			m_queryBuilder->recreateDependentQueries(typeIndex);
			m_tracker->recordChange(typeIndex);
			STEST_LOG_UNPRINTED(TESTLOG_ECS_STRUCTURAL_CHANGE_HAPPENED(typeIndex.name()))
		}
	};

	//container for ecs operations
	class EntityService
	{
		std::shared_ptr<ComponentStorage> m_componentStorage;
		std::shared_ptr<ComponentLookup> m_componentLookup;

		std::shared_ptr<QueryBuilder> m_queryBuilder;

		std::shared_ptr<StructuralChangeTracker> m_structuralChangeTracker;
		std::shared_ptr<StructuralChangeHandler> m_structuralChangeHandler;

		std::shared_ptr<EntityManager> m_entityManager;
		std::shared_ptr<ComponentManager> m_componentManager;
		std::shared_ptr<EntityEventManager> m_eventManager;

		spite::HeapAllocator m_allocator;

	public:
		EntityService(const ComponentAllocator& componentAllocator,
			const spite::HeapAllocator& heapAllocator) : m_allocator(heapAllocator)
		{
			m_componentStorage = std::make_shared<ComponentStorage>(componentAllocator);
			m_componentLookup = std::make_shared<ComponentLookup>(m_allocator);

			m_queryBuilder = std::make_shared<QueryBuilder>(m_componentLookup, m_componentStorage, m_allocator);

			m_structuralChangeTracker = std::make_shared<StructuralChangeTracker>(m_componentStorage, m_allocator);
			m_structuralChangeHandler = std::make_shared<StructuralChangeHandler>(
				m_queryBuilder, m_structuralChangeTracker);

			m_componentManager = std::make_shared<ComponentManager>(m_componentStorage, m_componentLookup,
				m_structuralChangeHandler);
			m_entityManager = std::make_shared<EntityManager>(m_componentStorage, m_componentLookup,
				m_structuralChangeHandler, m_allocator);

			m_eventManager = std::make_shared<EntityEventManager>(m_componentStorage, m_structuralChangeHandler);
		}

		template <t_plain_component TComponent>
		CommandBuffer<TComponent> getCommandBuffer()
		{
			return CommandBuffer<TComponent>(m_componentStorage.get(), m_componentLookup.get(), m_allocator,
				m_structuralChangeHandler);
		}

		[[nodiscard]] std::shared_ptr<EntityManager> entityManager() const
		{
			return m_entityManager;
		}

		[[nodiscard]] std::shared_ptr<ComponentStorage> componentStorage() const
		{
			return m_componentStorage;
		}

		[[nodiscard]] std::shared_ptr<ComponentLookup> componentLookup() const
		{
			return m_componentLookup;
		}

		[[nodiscard]] std::shared_ptr<ComponentManager> componentManager() const
		{
			return m_componentManager;
		}

		[[nodiscard]] std::shared_ptr<QueryBuilder> queryBuilder() const
		{
			return m_queryBuilder;
		}

		[[nodiscard]] std::shared_ptr<StructuralChangeTracker> structuralChangeTracker() const
		{
			return m_structuralChangeTracker;
		}

		[[nodiscard]] std::shared_ptr<EntityEventManager> entityEventManager()const
		{
			return m_eventManager;
		}


		[[nodiscard]] HeapAllocator allocator() const
		{
			return m_allocator;
		}

		[[nodiscard]] HeapAllocator* allocatorPtr() 
		{
			return &m_allocator;
		}

	};

	class SystemBase
	{
		EntityWorld* m_world = nullptr;

		eastl::vector<std::type_index, spite::HeapAllocator>* m_dependentTypes = nullptr;

		bool m_isActive = true;

		//world sets itself to system upon its addition 
		void setWorld(EntityWorld* world);

		friend class EntityWorld;

	protected:
		EntityService* m_entityService = nullptr;

		//call this method in onCreate so that system will run only when
		//component of specified type exists
		void requireComponent(const std::type_index typeIndex);

	public:
		SystemBase()
		{
		}

		bool isDependentOn(const std::type_index typeIndex) const;

		//true if depends on at least one type
		bool isDependentOn(const std::type_index* array, const sizet n) const;

		eastl::vector<std::type_index, HeapAllocator>& getDependencies();

		bool isActive() const;

		//called before anything else when system is added to world
		virtual void onInitialize();

		//called on all systems(enabled and disabled) when world is starting
		virtual void onStart();

		virtual void onEnable();

		virtual void onUpdate(float deltaTime);

		virtual void onFixedUpdate(float fixedDeltaTime);

		virtual void onLateUpdate(float deltaTime);

		virtual void onDisable();

		virtual void onDestroy();

		virtual ~SystemBase()
		{
			delete m_dependentTypes;
		}
	};

	//world
	//also manages systems' lifetime
	class EntityWorld
	{
		eastl::vector<SystemBase*, spite::HeapAllocator> m_allSystems;
		eastl::vector<SystemBase*, spite::HeapAllocator> m_activeSystems;

		EntityService* m_entityService;

		const spite::HeapAllocator m_allocator;

	public:
		EntityWorld(const ComponentAllocator& componentAllocator,
			const spite::HeapAllocator& heapAllocator) : m_allSystems(heapAllocator),
			m_activeSystems(heapAllocator),
			m_entityService(new EntityService(
				componentAllocator, heapAllocator)),
			m_allocator(heapAllocator)
		{
		}

		[[nodiscard]] EntityService* service() const
		{
			return m_entityService;
		}

		void start()
		{
			for (const auto system : m_allSystems)
			{
				system->onStart();
			}
		}

		void enable()
		{
			for (const auto system : m_activeSystems)
			{
				system->onEnable();
			}
		}

		void update(const float deltaTime)
		{
			for (const auto system : m_activeSystems)
			{
				system->onUpdate(deltaTime);
			}
		}

		void fixedUpdate(const float fixedDeltaTime)
		{
			for (const auto system : m_activeSystems)
			{
				system->onFixedUpdate(fixedDeltaTime);
			}
		}

		void lateUpdate(const float deltaTime)
		{
			for (const auto system : m_activeSystems)
			{
				system->onLateUpdate(deltaTime);
			}
		}

		//activate/deactivate systems that depend on component tables that where changed that frame
		//deactivate systems if their dependent components are not present and
		//activate inactive systems otherwise
		void commitSystemsStructuralChange()
		{
			auto tracker = m_entityService->structuralChangeTracker();

			auto& emptyTables = tracker->getEmptyTables();
			auto& nonEmptyTables = tracker->getNonEmptyTables();

			sizet emptyTablesCount = emptyTables.size();
			sizet nonEmptyTablesCount = nonEmptyTables.size();

			sizet systemsToDeactivateCount = 0;
			sizet systemsToActivateCount = 0;

			if (emptyTablesCount != 0)
			{
				for (SystemBase* system : m_activeSystems)
				{
					//if the system is currently active but has to be deactivated
					if (system->isDependentOn(emptyTables.data(), emptyTablesCount))
					{
						system->m_isActive = false;
						system->onDisable();
						++systemsToDeactivateCount;
					}
				}
			}
			if (nonEmptyTablesCount != 0)
			{
				auto storage = m_entityService->componentStorage();
				for (SystemBase* system : m_allSystems)
				{
					//need inactive and dependent systems
					if (system->isActive() || !system->isDependentOn(nonEmptyTables.data(), nonEmptyTablesCount))
					{
						continue;
					}

					auto& dependencies = system->getDependencies();
					bool canBeActivated = true;
					for (const std::type_index& dependency : dependencies)
					{
						canBeActivated &= storage->hasAnyComponents(dependency);
					}

					//enable system
					if (canBeActivated)
					{
						system->m_isActive = true;
						system->onEnable();
						++systemsToActivateCount;
					}
				}
			}

			if (systemsToActivateCount == 0 && systemsToDeactivateCount == 0)
			{
				tracker->reset();
				return;
			}

			sizet newActiveSystemsCount = m_activeSystems.size() + systemsToActivateCount - systemsToDeactivateCount;
			m_activeSystems.resize(newActiveSystemsCount);

			sizet iter = 0;
			for (SystemBase* system : m_allSystems)
			{
				if (system->isActive())
				{
					m_activeSystems[iter] = system;
					++iter;
				}
			}
			SASSERTM(iter == newActiveSystemsCount,
				"Something went wrong upon system state restructure, expected %i, actual %i",
				newActiveSystemsCount, iter)

				tracker->reset();
		}


		//the order of added systems is the order by which they will be updated
		void addSystems(SystemBase** array, const sizet n)
		{
			m_allSystems.reserve(n);

			for (sizet i = 0; i < n; ++i)
			{
				addSystem(array[i]);
			}
		}

		//the order of added systems is the order by which they will be updated
		void addSystem(SystemBase* system)
		{
			SASSERTM(system->m_world == nullptr, "System is already owned by another world")
				system->setWorld(this);
			m_allSystems.push_back(system);
			system->onInitialize();

			if (setSystemStatus(system))
			{
				m_activeSystems.push_back(system);
			}
		}

		//TODO: track and delete cached queries
		//does not delete cached queries (yet)
		void destroySystem(SystemBase* system)
		{
			m_allSystems.erase_first(system);
			m_activeSystems.erase_first(system);

			system->onDestroy();
			delete system;
			system = nullptr;
		}

		~EntityWorld()
		{
			for (auto& system : m_allSystems)
			{
				system->onDestroy();
				delete system;
			}
			m_allSystems.clear();
			m_activeSystems.clear();
			delete m_entityService;
		}

	private:
		//set system status upon its addition to world
		bool setSystemStatus(SystemBase* system) const
		{
			auto& dependencies = system->getDependencies();

			if (dependencies.size() == 0)
			{
				system->m_isActive = true;
				return true;
			}

			auto storage = m_entityService->componentStorage();
			for (const std::type_index dependency : dependencies)
			{
				if (!storage->hasAnyComponents(dependency))
				{
					system->m_isActive = false;
					return false;
				}
			}
			system->m_isActive = true;
			return true;
		}
	};

	inline void SystemBase::setWorld(EntityWorld* world)
	{
		m_world = world;
		m_entityService = m_world->service();

		m_dependentTypes = new eastl::vector<std::type_index, HeapAllocator>(m_entityService->allocator());
	}

	inline void SystemBase::requireComponent(const std::type_index typeIndex)
	{
		m_dependentTypes->push_back(typeIndex);
	}

	inline bool SystemBase::isDependentOn(const std::type_index typeIndex) const
	{
		return eastl::find(m_dependentTypes->begin(), m_dependentTypes->end(), typeIndex) != m_dependentTypes->end();
	}

	inline bool SystemBase::isDependentOn(const std::type_index* array, const sizet n) const
	{
		if (m_dependentTypes->size() == 0 || n == 0)
		{
			return false;
		}

		bool result = false;
		for (sizet i = 0; i < n; ++i)
		{
			result |= isDependentOn(array[i]);
		}

		return result;
	}

	inline eastl::vector<std::type_index, HeapAllocator>& SystemBase::getDependencies()
	{
		return *m_dependentTypes;
	}

	inline bool SystemBase::isActive() const
	{
		return m_isActive;
	}

	inline void SystemBase::onInitialize()
	{
	}

	inline void SystemBase::onStart()
	{
	}

	inline void SystemBase::onEnable()
	{
	}

	inline void SystemBase::onUpdate(float deltaTime)
	{
	}

	inline void SystemBase::onFixedUpdate(float fixedDeltaTime)
	{
	}

	inline void SystemBase::onLateUpdate(float deltaTime)
	{
	}

	inline void SystemBase::onDisable()
	{
	}

	inline void SystemBase::onDestroy()
	{
	}
}
