#pragma once

#include "ecs/systems/SystemDependencies.hpp"
#include "ecs/core/EntityManager.hpp"
#include "ecs/cbuffer/CommandBuffer.hpp"
#include "ecs/query/QueryBuilder.hpp"

namespace spite
{
	//a wrapper for QueryBuilder to use within Systems
	class SystemQueryBuilder
	{
	private:
		QueryBuilder m_queryBuilder;

		friend class SystemBase;
	public:
		SystemQueryBuilder(QueryBuilder queryBuilder): m_queryBuilder(std::move(queryBuilder))
		{
		}

		template <typename T>
		SystemQueryBuilder& with()
		{
			m_queryBuilder.with<T>();
			return *this;
		}

		template <typename T>
		SystemQueryBuilder& with_write()
		{
			m_queryBuilder.with_write<T>();
			return *this;
		}

		template <typename T>
		SystemQueryBuilder& with_read()
		{
			m_queryBuilder.with_read<T>();
			return *this;
		}

		template <typename T>
		SystemQueryBuilder& without()
		{
			m_queryBuilder.without<T>();
			return *this;
		}

		template <typename T>
		SystemQueryBuilder& enabled()
		{
			m_queryBuilder.enabled<T>();
			return *this;
		}

		template <typename T>
		SystemQueryBuilder& modified()
		{
			m_queryBuilder.modified<T>();
			return *this;
		}
	};

	// A context object passed to systems during their execution.
	// It provides a safe, verified wrapper around the EntityManager, preventing
	// direct structural changes and enforcing dependency declarations.
	class SystemContext
	{
	private:
		EntityManager* m_entityManager{};
		const SystemDependencies* m_dependencies{};
		CommandBuffer* cb{};

	public:
		float deltaTime{};

		SystemContext() = default;

		SystemContext(EntityManager* entityManager, CommandBuffer* commandBuffer, float dt,
		              const SystemDependencies* deps)
			: m_entityManager(entityManager), m_dependencies(deps), cb(commandBuffer), deltaTime(dt)
		{
		}

		CommandBuffer& getCommandBuffer() const
		{
			SASSERT(cb)
			return *cb;
		}

		SystemQueryBuilder getQueryBuilder() const
		{
			return SystemQueryBuilder(m_entityManager->getQueryBuilder());
		}

		template <t_component T>
		const T* tryGetComponent(Entity entity) const
		{
			if (hasComponent<T>(entity))
			{
				return &getComponent<T>(entity);
			}
			return nullptr;
		}

		template <t_component T>
		T* tryGetComponent(Entity entity)
		{
			if (hasComponent<T>(entity))
			{
				return &getComponent<T>(entity);
			}
			return nullptr;
		}

		template <t_component T>
		T& getComponent(Entity entity)
		{
			const ComponentID componentId = ComponentMetadataRegistry::getComponentId<T>();
			SASSERTM(m_dependencies && m_dependencies->read.test(componentId),
			         "System attempted a WRITE on component '%s' which it did not declare with Write<T>.",
			         typeid(T).name())
			return m_entityManager->getComponent<T>(entity);
		}

		template <t_component T>
		const T& getComponent(Entity entity) const
		{
			const ComponentID componentId = ComponentMetadataRegistry::getComponentId<T>();
			SASSERTM(
				m_dependencies && (m_dependencies->read.test(componentId) || m_dependencies->write.test(
					componentId)),
				"System attempted a READ on component '%s' which it did not declare a dependency on.",
				typeid(T).name())
			return m_entityManager->getComponent<T>(entity);
		}

		template <t_singleton_component T>
		T& getSingletonComponent()
		{
			return m_entityManager->getSingletonComponent<T>();
		}

		template <t_singleton_component T>
		const T& getSingletonComponent() const
		{
			return m_entityManager->getSingletonComponent<T>();
		}

		bool isEntityValid(Entity entity) const
		{
			return m_entityManager->isValid(entity);
		}

		template <t_component T>
		bool hasComponent(Entity entity) const
		{
			return m_entityManager->hasComponent<T>(entity);
		}
	};
}
