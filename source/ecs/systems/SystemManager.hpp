#pragma once

#include "ecs/systems/SystemBase.hpp"
#include "base/CollectionAliases.hpp"
#include <memory>
#include <utility>

#include <enkiTS/TaskScheduler.h>

namespace enki
{
	class TaskScheduler;
}

namespace spite
{
	class EntityManager;

	struct SystemTask : enki::ITaskSet
	{
		SystemBase* system = nullptr;
		SystemContext systemContext{};
		float deltaTime = 0.0f;

		enki::Dependency dependency;

		SystemTask() = default;

		SystemTask(SystemBase* system, const SystemContext& context, float dt) : system(system),
			systemContext(context),
			deltaTime(dt)
		{
		}

		SystemTask(const SystemTask& other) = delete;

		SystemTask(SystemTask&& other) noexcept: system(other.system), systemContext(other.systemContext),
		                                         deltaTime(other.deltaTime), dependency(std::move(other.dependency))
		{
		}

		SystemTask& operator=(const SystemTask& other) = delete;
		SystemTask& operator=(SystemTask&& other) = delete;

		~SystemTask() override = default;

		void ExecuteRange(enki::TaskSetPartition range, u32 threadnum) override
		{
			system->onUpdate(systemContext);
		}
	};

	struct SystemGraph
	{
		heap_unordered_map<SystemBase*, heap_vector<SystemBase*>> graph;
		heap_unordered_map<SystemBase*, u32> incomingDependencyCount;

		SystemGraph(const HeapAllocator& allocator) :
			graph(makeHeapMap<SystemBase*, heap_vector<SystemBase*>>(allocator)),
			incomingDependencyCount(makeHeapMap<SystemBase*, u32>(allocator))
		{
		}
	};

	class SystemManager
	{
	private:
		EntityManager* m_entityManager;
		SystemDependencyStorage m_dependencyStorage;
		AspectRegistry* m_aspectRegistry;
		VersionManager* m_versionManager;
		HeapAllocator m_allocator;

		heap_vector<CommandBuffer> m_commandBuffers;
		heap_vector<ExecutionStage> m_executionStages;
		heap_unordered_map<ExecutionStage, SystemGraph> m_cachedMasterGraphs;

		std::unique_ptr<enki::TaskScheduler> m_taskScheduler;

		heap_vector<std::unique_ptr<SystemBase>> m_systems;

		bool m_isInitialized = false;

		void buildAndExecuteStage(ExecutionStage stage, CommandBuffer* commandBuffer, float deltaTime);

		void buildDependencyGraph(eastl::span<SystemBase*> systemsInStage,
		                          SystemGraph& systemGraph);

	public:
		SystemManager(const HeapAllocator& allocator, EntityManager* entityManager,
		              AspectRegistry* aspectRegistry, VersionManager* versionManager);

		void registerSystem(std::unique_ptr<SystemBase> system);

		void registerSystems(eastl::span<std::unique_ptr<SystemBase>> systems);

		template <typename T>
		void registerSystem()
		{
			static_assert(std::is_base_of_v<SystemBase, T>);
			registerSystem(std::make_unique<T>());
		}

		template <typename ...T>
		void registerSystems()
		{
			(registerSystem<T>(), ...);
		}

		// Call this after all systems are registered, before the first update.
		void initialize();

		void update(float deltaTime);
	};
}
