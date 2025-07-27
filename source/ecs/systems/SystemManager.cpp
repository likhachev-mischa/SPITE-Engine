#include "SystemManager.hpp"
#include "ecs/core/EntityManager.hpp"
#include "ecs/storage/AspectRegistry.hpp"

namespace spite
{
	void SystemManager::initialize()
	{
		SASSERTM(!m_isInitialized, "SystemManager is already initialized.")

		for (const auto& stage : m_executionStages)
		{
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
				buildDependencyGraph(systemsInStage, m_cachedMasterGraphs.at(stage));
			}
		}

		m_isInitialized = true;
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
	}

	SystemManager::SystemManager(const HeapAllocator& allocator, EntityManager* entityManager,
	                             AspectRegistry* aspectRegistry, VersionManager* versionManager,
	                             eastl::span<ExecutionStage> executionStages)
		: m_entityManager(entityManager), m_dependencyStorage(allocator),
		  m_aspectRegistry(aspectRegistry),
		  m_versionManager(versionManager),
		  m_allocator(allocator),
		  m_commandBuffers(
			  makeHeapVector<CommandBuffer>(allocator)),
		  m_executionStages(
			  makeHeapVector<ExecutionStage>(allocator)),
		  m_cachedMasterGraphs(makeHeapMap<ExecutionStage, SystemGraph>(allocator)),
		  m_systems(makeHeapVector<std::unique_ptr<SystemBase>>(allocator))
	{
		for (const auto& stage : executionStages)
		{
			if (std::ranges::find(m_executionStages, stage) == m_executionStages.end())
			{
				m_executionStages.emplace_back(stage);
				m_commandBuffers.emplace_back(entityManager->getCommandBuffer());
			}
		}

		m_taskScheduler = std::make_unique<enki::TaskScheduler>();
		m_taskScheduler->Initialize();
	}

	void SystemManager::registerSystem(std::unique_ptr<SystemBase> system)
	{
		SASSERTM(!m_isInitialized, "Cannot register systems after initialization.")
		SystemContext initCtx(m_entityManager, nullptr, 0.0f, nullptr);
		system->onInitialize(initCtx, m_dependencyStorage);
		m_systems.push_back(std::move(system));
	}

	void SystemManager::registerSystems(eastl::span<std::unique_ptr<SystemBase>> systems)
	{
		SASSERTM(!m_isInitialized, "Cannot register systems after initialization.")
		m_systems.reserve(systems.size());

		SystemContext ctx(m_entityManager, nullptr, 0.0f, nullptr);
		for (auto& system : systems)
		{
			system->onInitialize(ctx, m_dependencyStorage);
			m_systems.push_back(std::move(system));
		}
	}

	void SystemManager::update(float deltaTime)
	{
		SASSERTM(m_isInitialized, "SystemManager is not initialized. Call initialize() after registering all systems.")
		for (sizet i = 0; i < m_executionStages.size(); ++i)
		{
			buildAndExecuteStage(m_executionStages[i], &m_commandBuffers[i], deltaTime);
			m_commandBuffers[i].commit(*m_entityManager);
		}
	}
}
