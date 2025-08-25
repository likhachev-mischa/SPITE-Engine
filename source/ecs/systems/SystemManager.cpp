#include "SystemManager.hpp"
#include "ecs/core/EntityManager.hpp"
#include "ecs/storage/AspectRegistry.hpp"
#include "base/Logging.hpp"
#include <typeinfo>
#include <algorithm>

namespace spite
{
	void SystemManager::initialize()
	{
		SASSERTM(!m_isInitialized, "SystemManager is already initialized.")
		SDEBUG_LOG("\n --- Initializing System Graph ---\n")

		eastl::sort(m_executionStages.begin(), m_executionStages.end());

		for (const auto& stage : m_executionStages)
		{
			m_commandBuffers.emplace_back(m_entityManager->createCommandBuffer());

			auto systemsInStage = makeScratchVector<SystemBase*>(FrameScratchAllocator::get());
			for (const auto& system : m_systems)
			{
				if (system->getExecutionStage() == stage)
				{
					systemsInStage.push_back(system.get());
				}
			}

			if (!systemsInStage.empty())
			{
				m_cachedMasterGraphs.emplace(stage, SystemGraph(m_allocator));
				SDEBUG_LOG("Building System Graph for %i stage\n", stage)
				buildDependencyGraph(systemsInStage, m_cachedMasterGraphs.at(stage));
			}
		}

		m_isInitialized = true;
		SDEBUG_LOG("\n--- System Graph Initialized ---\n\n")
	}

	void SystemManager::buildAndExecuteStage(ExecutionStage stage, CommandBuffer* commandBuffer, float deltaTime)
	{
		SASSERTM(m_isInitialized, "SystemManager is not initialized. Call initialize() after registering all systems.")

		auto masterGraphIt = m_cachedMasterGraphs.find(stage);
		if (masterGraphIt == m_cachedMasterGraphs.end()) return; // No systems in this stage

		const auto& masterGraph = masterGraphIt->second;

		auto marker = FrameScratchAllocator::get().get_scoped_marker();
		auto activeSystems = makeScratchVector<SystemBase*>(FrameScratchAllocator::get());
		for (const auto& system : m_systems)
		{
			SystemContext context(m_entityManager, commandBuffer, deltaTime,
			                      &m_dependencyStorage.getDependencies(system.get()));

			system->prepareForUpdate(context, *m_versionManager);
			if (system->getExecutionStage() == stage && system->isActive())
			{
				activeSystems.push_back(system.get());
			}
		}

		if (activeSystems.empty()) return;

		auto tasks = makeScratchMap<SystemBase*, SystemTask>(FrameScratchAllocator::get());
		auto activeIncomingCounts = makeScratchMap<SystemBase*, u32>(FrameScratchAllocator::get());

		for (auto* system : activeSystems)
		{
			SystemContext context(m_entityManager, commandBuffer, deltaTime,
			                      &m_dependencyStorage.getDependencies(system));
			tasks.emplace(system, SystemTask(system, context, deltaTime));
			activeIncomingCounts[system] = 0; // Initialize active count
		}

		// Build active graph and set dependencies
		for (auto* system : activeSystems)
		{
			auto& task = tasks.at(system);
			const auto& masterSuccessors = masterGraph.graph.at(system);
			for (auto* successor : masterSuccessors)
			{
				if (tasks.count(successor)) // If the successor is also active
				{
					tasks.at(successor).SetDependency(tasks.at(successor).dependency, &task);
					activeIncomingCounts[successor]++;
				}
			}
		}

		// Schedule root tasks
		for (auto* system : activeSystems)
		{
			if (activeIncomingCounts.at(system) == 0)
			{
				m_taskScheduler->AddTaskSetToPipe(&tasks.at(system));
			}
		}

		m_taskScheduler->WaitforAll();
	}

	void SystemManager::buildDependencyGraph(eastl::span<SystemBase*> systemsInStage,
	                                         SystemGraph& systemGraph)
	{
		// 1. Initialize graph and incoming counts
		for (auto* sys : systemsInStage)
		{
			systemGraph.graph.emplace(sys, makeHeapVector<SystemBase*>(m_allocator));
			systemGraph.incomingDependencyCount[sys] = 0;
		}

		// 2. Build graph by checking all pairs for conflicts.
		for (sizet i = 0; i < systemsInStage.size(); ++i)
		{
			for (sizet j = i + 1; j < systemsInStage.size(); ++j)
			{
				SystemBase* sysA = systemsInStage[i];
				SystemBase* sysB = systemsInStage[j];

				const auto& depsA = m_dependencyStorage.getDependencies(sysA);
				const auto& depsB = m_dependencyStorage.getDependencies(sysB);

				bool hasConflict = false;

				// A conflict exists if one system writes to a component that another system reads or writes.
				const auto& readA = depsA.read;
				const auto& writeA = depsA.write;
				const auto& readB = depsB.read;
				const auto& writeB = depsB.write;

				if ((writeA & readB).any() || (writeA & writeB).any() || (writeB & readA).any())
				{
					hasConflict = true;
				}


				if (hasConflict)
				{
					// A conflict exists. Use the tuple order to create the dependency.
					// Since i < j, sysA has higher priority and must run before sysB.
					systemGraph.graph.at(sysA).push_back(sysB);
					systemGraph.incomingDependencyCount[sysB]++;
				}
			}
		}

		SDEBUG_LOG("--- System Dependency Graph ---\n")
		for (auto* system : systemsInStage)
		{
			SDEBUG_LOG("System '%s' has %u dependencies", typeid(*system).name(),
			           systemGraph.incomingDependencyCount.at(system))
			auto& successors = systemGraph.graph.at(system);
			if (!successors.empty())
			{
				SDEBUG_LOG("  Successors:")
				for (auto* successor : successors)
				{
					SDEBUG_LOG("    - %s", typeid(*successor).name())
				}
			}
			SDEBUG_LOG("\n")
		}
		SDEBUG_LOG("-----------------------------\n")
	}

	SystemManager::SystemManager(const HeapAllocator& allocator, EntityManager* entityManager,
	                             VersionManager* versionManager)
		: m_entityManager(entityManager), m_dependencyStorage(allocator),
		  m_versionManager(versionManager),
		  m_allocator(allocator),
		  m_commandBuffers(
			  makeHeapVector<CommandBuffer>(allocator)),
		  m_executionStages(
			  makeHeapVector<ExecutionStage>(allocator)),
		  m_cachedMasterGraphs(makeHeapMap<ExecutionStage, SystemGraph>(allocator)),
		  m_systems(makeHeapVector<std::unique_ptr<SystemBase>>(allocator))
	{
		m_taskScheduler = std::make_unique<enki::TaskScheduler>();
		m_taskScheduler->Initialize();
	}

	void SystemManager::registerSystem(std::unique_ptr<SystemBase> system)
	{
		SASSERTM(!m_isInitialized, "Cannot register systems after initialization.")
		SystemContext initCtx(m_entityManager, nullptr, 0.0f, nullptr);
		system->onInitialize(initCtx, m_dependencyStorage);

		if (std::ranges::find(m_executionStages, system->getExecutionStage()) == m_executionStages.end())
		{
			m_executionStages.push_back(system->getExecutionStage());
		}

		m_systems.push_back(std::move(system));
	}

	void SystemManager::registerSystems(eastl::span<std::unique_ptr<SystemBase>> systems)
	{
		//TODO introduce block allocator
		SASSERTM(!m_isInitialized, "Cannot register systems after initialization.")
		m_systems.reserve(systems.size());

		SystemContext ctx(m_entityManager, nullptr, 0.0f, nullptr);
		for (auto& system : systems)
		{
			system->onInitialize(ctx, m_dependencyStorage);
			if (std::ranges::find(m_executionStages, system->getExecutionStage()) == m_executionStages.end())
			{
				m_executionStages.push_back(system->getExecutionStage());
			}
			m_systems.push_back(std::move(system));
		}
	}

	void SystemManager::update(float deltaTime)
	{
		SASSERTM(m_isInitialized, "SystemManager is not initialized. Call initialize() after registering all systems.")

		for (sizet i = 0; i < m_executionStages.size(); ++i)
		{
			//we need to reset modifications after the normal update stages, but before rendering
			//TODO maybe move this somewhere else
			if (m_executionStages[i] == CoreExecutionStages::PRE_RENDER)
			{
				m_entityManager->getArchetypeManager()->resetAllModificationTracking();
			}

			//parallel exec
			//buildAndExecuteStage(m_executionStages[i], &m_commandBuffers[i], deltaTime);

			//serial exec for debug
			for (const auto& system : m_systems)
			{
				if (system->getExecutionStage() == m_executionStages[i])
				{
					SystemContext context(m_entityManager, &m_commandBuffers[i], deltaTime,
					                      &m_dependencyStorage.getDependencies(system.get()));
					system->prepareForUpdate(context, *m_versionManager);
					system->onUpdate(context);
				}
			}

			m_commandBuffers[i].commit(*m_entityManager);
		}
	}
}
