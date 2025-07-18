#pragma once
#include "ecs/core/EntityManager.hpp"
#include "ecs/query/QueryHandle.hpp"
#include "ecs/storage/VersionManager.hpp"
#include <optional>

namespace spite
{
	// Forward declare for the friend class declaration.
	class SystemManager;

	class SystemBase
	{
		friend class SystemManager;

	private:
		bool m_isActive = true;
		bool m_isManuallyDisabled = false;

		VersionManager* m_versionManager;

		std::optional<QueryHandle> m_prerequisite;

		bool m_isPrerequisiteMet = false;
		bool m_wasPrerequisiteMet = false;
		u64 m_cachedVersion = 0;

	protected:
		EntityManager* m_entityManager;

	public:
		SystemBase(EntityManager* entityManager, VersionManager* versionManager)
			: m_versionManager(versionManager), m_entityManager(entityManager)
		{
		}

		virtual ~SystemBase() = default;

		bool isActive() const
		{
			return m_isActive;
		}

		//method for user-controlled system state
		void enable()
		{
			m_isManuallyDisabled = false;
		}

		//method for user-controlled system state
		void disable()
		{
			m_isManuallyDisabled = true;
		}

	protected:
		void setPrerequisite(QueryHandle prerequisite)
		{
			m_prerequisite = std::move(prerequisite);
			// Initial check is done in updatePrerequisiteState() which will be
			// called by the SystemManager before the first update.
		}

		virtual void onEnable()
		{
		}

		virtual void onDisable()
		{
		}

		virtual void onInitialize()
		{
		};

		virtual void onUpdate(float deltaTime)
		{
		};

		virtual void onLateUpdate(float deltaTime)
		{
			
		}

		virtual void onFixedUpdate(float deltaTime)
		{
			
		}

		virtual void onDestroy()
		{
		};

	private:
		// This method's ONLY job is to keep m_isPrerequisiteMet up-to-date.
		void updatePrerequisiteState()
		{
			m_wasPrerequisiteMet = m_isPrerequisiteMet;
			if (!m_prerequisite.has_value())
			{
				m_isPrerequisiteMet = true; // No prerequisite always means met.
				return;
			}

			const Aspect* aspect = m_prerequisite->getDescriptor().includeAspect;
			const u64 currentVersion = m_versionManager->getVersion(*aspect);

			if (currentVersion != m_cachedVersion)
			{
				m_isPrerequisiteMet = m_prerequisite->getEntityCount() > 0;
				m_cachedVersion = currentVersion;
			}
		}

		// must be called every frame for every system before update.
		void prepareForUpdate()
		{
			if (m_isManuallyDisabled)
			{
				m_isActive = false;
				return;
			}

			updatePrerequisiteState();

			if (m_isPrerequisiteMet && !m_wasPrerequisiteMet)
			{
				m_isActive = true;
				onEnable();
			}
			else if (!m_isPrerequisiteMet && m_wasPrerequisiteMet)
			{
				m_isActive = false;
				onDisable();
			}
		}
	};
}
