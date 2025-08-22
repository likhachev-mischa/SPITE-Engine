#pragma once
#include <optional>
#include <typeindex>
#include <typeinfo>

#include "SystemDependencyStorage.hpp"
#include "SystemContext.hpp"

#include "ecs/core/IComponent.hpp"
#include "ecs/query/QueryHandle.hpp"
#include "ecs/systems/ExecutionStage.hpp"

namespace spite
{
	class SystemManager;
	class EntityManager;
	class QueryBuilder;

	class SystemBase
	{
		friend class SystemManager;

	private:
		bool m_isActive = true;
		bool m_isManuallyDisabled = false;

		QueryHandle m_prerequisite{};

		bool m_isPrerequisiteMet = false;
		bool m_wasPrerequisiteMet = false;
		u64 m_cachedVersion = 0;

		ExecutionStage m_stage = CoreExecutionStages::UPDATE;

	protected:
		QueryHandle registerQuery(SystemQueryBuilder& builder, SystemDependencyStorage& dependencyStorage);

		template <typename... T>
		void declareAccess(SystemDependencyStorage& dependencyStorage);

		void setExecutionStage(ExecutionStage stage)
		{
			m_stage = stage;
		}

	public:
		SystemBase() = default;
		virtual ~SystemBase() = default;

		bool isActive() const;
		void enable();
		void disable();

		virtual void onEnable(SystemContext ctx);
		virtual void onDisable(SystemContext ctx);
		virtual void onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage);
		virtual void onUpdate(SystemContext ctx);
		virtual void onLateUpdate(SystemContext ctx);
		virtual void onFixedUpdate(SystemContext ctx);
		virtual void onDestroy(SystemContext ctx);

		ExecutionStage getExecutionStage() const { return m_stage; }

	protected:
		void setPrerequisite(QueryHandle prerequisite);

	private:
		void updatePrerequisiteState(const VersionManager& versionManager);
		void prepareForUpdate(const SystemContext& ctx, const VersionManager& versionManager);
	};

	template <typename... T>
	void SystemBase::declareAccess(SystemDependencyStorage& dependencyStorage)
	{
		static_assert(((is_read_wrapper_v<T> || is_write_wrapper_v<T>) && ...),
		              "declareAccess must be called with Read<T> or Write<T> wrappers.");
		dependencyStorage.registerDependencies(
			this, {
				(is_read_wrapper_v<T>
					 ? ComponentMetadataRegistry::getComponentId<typename T::type>()
					 : INVALID_COMPONENT_ID)...
			}, {
				(is_write_wrapper_v<T>
					 ? ComponentMetadataRegistry::getComponentId<typename T::type>()
					 : INVALID_COMPONENT_ID)...
			});
	}
}
