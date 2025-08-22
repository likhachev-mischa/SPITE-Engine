#include "SystemBase.hpp"
#include "ecs/query/QueryBuilder.hpp"

namespace spite
{
	QueryHandle SystemBase::registerQuery(SystemQueryBuilder& builder, SystemDependencyStorage& dependencyStorage)
	{
		QueryHandle handle = builder.m_queryBuilder.build();
		dependencyStorage.registerQuery(this, handle.getDescriptor());
		return handle;
	}

	void SystemBase::enable()
	{
		m_isManuallyDisabled = false;
	}

	void SystemBase::disable()
	{
		m_isManuallyDisabled = true;
	}

	void SystemBase::setPrerequisite(QueryHandle prerequisite)
	{
		m_prerequisite = prerequisite;
		// Initial check is done in updatePrerequisiteState() which will be
		// called by the SystemManager before the first update.
	}

	void SystemBase::onEnable(SystemContext ctx)
	{
	}

	void SystemBase::onDisable(SystemContext ctx)
	{
	}

	void SystemBase::onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage)
	{
	}

	void SystemBase::onUpdate(SystemContext ctx)
	{
	}

	void SystemBase::onLateUpdate(SystemContext ctx)
	{
	}

	void SystemBase::onFixedUpdate(SystemContext ctx)
	{
	}

	void SystemBase::onDestroy(SystemContext ctx)
	{
	}

	void SystemBase::updatePrerequisiteState(const VersionManager& versionManager)
	{
		m_wasPrerequisiteMet = m_isPrerequisiteMet;
		if (!m_prerequisite.isValid())
		{
			m_isPrerequisiteMet = true; // No prerequisite always means met.
			return;
		}

		const Aspect* aspect = m_prerequisite.getDescriptor().includeAspect;
		const u64 currentVersion = versionManager.getVersion(*aspect);

		if (currentVersion != m_cachedVersion)
		{
			m_isPrerequisiteMet = m_prerequisite.getEntityCount() > 0;
			m_cachedVersion = currentVersion;
		}
	}

	void SystemBase::prepareForUpdate(const SystemContext& ctx, const VersionManager& versionManager)
	{
		if (m_isManuallyDisabled)
		{
			m_isActive = false;
			return;
		}

		updatePrerequisiteState(versionManager);

		if (m_isPrerequisiteMet && !m_wasPrerequisiteMet)
		{
			m_isActive = true;
			onEnable(ctx);
		}
		else if (!m_isPrerequisiteMet && m_wasPrerequisiteMet)
		{
			m_isActive = false;
			onDisable(ctx);
		}
	}

	bool SystemBase::isActive() const
	{
		return m_isActive;
	}
}
