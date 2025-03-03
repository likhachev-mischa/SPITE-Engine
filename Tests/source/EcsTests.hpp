#pragma once
#include <ecs/World.hpp>
#include <ecs/Core.hpp>

#include <gtest/gtest.h>


namespace test1
{
	using namespace spite;

	struct Component : IComponent
	{
		sizet data;

		Component() : IComponent() { data = 0; }

		Component(const Component& other)
			: IComponent(other),
			  data(other.data)
		{
		}

		Component(Component&& other) noexcept
			: IComponent(std::move(other)),
			  data(other.data)
		{
		}

		Component& operator=(const Component& other)
		{
			if (this == &other)
				return *this;
			IComponent::operator =(other);
			data = other.data;
			return *this;
		}

		Component& operator=(Component&& other) noexcept
		{
			if (this == &other)
				return *this;

			data = other.data;
			IComponent::operator =(std::move(other));
			return *this;
		}

		~Component() override = default;
	};

	struct UpdateCountComponent : IComponent
	{
		sizet count;

		UpdateCountComponent(): count(0)
		{
		}

		UpdateCountComponent(const UpdateCountComponent& other)
			: IComponent(other),
			  count(other.count)
		{
		}

		UpdateCountComponent(UpdateCountComponent&& other) noexcept
			: IComponent(std::move(other)),
			  count(other.count)
		{
		}

		UpdateCountComponent& operator=(const UpdateCountComponent& other)
		{
			if (this == &other)
				return *this;
			IComponent::operator =(other);
			count = other.count;
			return *this;
		}

		UpdateCountComponent& operator=(UpdateCountComponent&& other) noexcept
		{
			if (this == &other)
				return *this;

			count = other.count;
			IComponent::operator =(std::move(other));
			return *this;
		}

		~UpdateCountComponent() override = default;
	};

	//entites which are created upon 1st system init
	constexpr sizet CREATED_ENTITIES_COUNT = 15;

	class CreateComponentSystem : public SystemBase
	{
		Query1<UpdateCountComponent>* m_query{};

	public:
		void onInitialize() override
		{
			Entity entity = m_entityService->entityManager()->createEntity();
			m_entityService->componentManager()->addComponent<UpdateCountComponent>(entity);

			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			m_query = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);

			sizet entityCount = CREATED_ENTITIES_COUNT;
			for (sizet i = 0; i < entityCount; ++i)
			{
				entity = m_entityService->entityManager()->createEntity();
				m_entityService->componentManager()->addComponent<Component>(entity);
			}
		}

		void onUpdate(float deltaTime) override
		{
			auto& query = *m_query;
			for (auto& updateCountComponent : query)
			{
				++updateCountComponent.count;
			}
		}
	};

	class WriteComponentSystem : public SystemBase
	{
	private:
		Query1<Component>* m_query{};
		std::type_index m_type = std::type_index(typeid(Component));

		Query1<UpdateCountComponent>* m_updateQuery{};

	public:
		void onInitialize() override
		{
			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			m_query = queryBuilder->buildQuery<Component>(buildInfo);

			buildInfo = queryBuilder->getQueryBuildInfo();
			m_updateQuery = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);
		}

		void onUpdate(float deltaTime) override
		{
			sizet querySize = m_query->getSize();


			auto& updateQuery = *m_updateQuery;
			sizet updateIteration = 0;

			for (const auto& updateCountComponent : updateQuery)
			{
				updateIteration = updateCountComponent.count;
			}


			auto& query = *m_query;
			for (auto& componentA : query)
			{
				componentA.data = updateIteration;
			}
		}
	};

	class ReadComponentSystem : public SystemBase
	{
	private:
		Query1<Component>* m_query{};
		std::type_index m_type = std::type_index(typeid(Component));


		Query1<UpdateCountComponent>* m_updateQuery{};

	public:
		void onInitialize() override
		{
			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			m_query = queryBuilder->buildQuery<Component>(buildInfo);

			buildInfo = queryBuilder->getQueryBuildInfo();
			m_updateQuery = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);
		}

		void onUpdate(float deltaTime) override
		{
			auto& updateQuery = *m_updateQuery;
			sizet updateIteration = 0;

			for (const auto& updateCountComponent : updateQuery)
			{
				updateIteration = updateCountComponent.count;
			}

			auto& query = *m_query;
			for (auto& componentA : query)
			{
				EXPECT_EQ(componentA.data, updateIteration);
			}

			++updateIteration;
		}
	};
}

namespace test2
{
	using namespace spite;

	struct Component : IComponent
	{
		sizet data;

		Component() : IComponent() { data = 0; }

		Component(const Component& other)
			: IComponent(other),
			  data(other.data)
		{
		}

		Component(Component&& other) noexcept
			: IComponent(std::move(other)),
			  data(other.data)
		{
		}

		Component& operator=(const Component& other)
		{
			if (this == &other)
				return *this;
			IComponent::operator =(other);
			data = other.data;
			return *this;
		}

		Component& operator=(Component&& other) noexcept
		{
			if (this == &other)
				return *this;

			data = other.data;
			IComponent::operator =(std::move(other));
			return *this;
		}

		~Component() override = default;
	};

	struct UpdateCountComponent : IComponent
	{
		sizet count;

		UpdateCountComponent(): count(0)
		{
		}

		UpdateCountComponent(const UpdateCountComponent& other)
			: IComponent(other),
			  count(other.count)
		{
		}

		UpdateCountComponent(UpdateCountComponent&& other) noexcept
			: IComponent(std::move(other)),
			  count(other.count)
		{
		}

		UpdateCountComponent& operator=(const UpdateCountComponent& other)
		{
			if (this == &other)
				return *this;
			IComponent::operator =(other);
			count = other.count;
			return *this;
		}

		UpdateCountComponent& operator=(UpdateCountComponent&& other) noexcept
		{
			if (this == &other)
				return *this;

			count = other.count;
			IComponent::operator =(std::move(other));
			return *this;
		}

		~UpdateCountComponent() override = default;
	};

	class CreateComponentSystem : public SystemBase
	{
		Query1<UpdateCountComponent>* m_query{};

	public:
		void onInitialize() override
		{
			Entity entity = m_entityService->entityManager()->createEntity();
			m_entityService->componentManager()->addComponent<UpdateCountComponent>(entity);

			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			m_query = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);
		}

		void onUpdate(float deltaTime) override
		{
			Entity entity = m_entityService->entityManager()->createEntity();
			m_entityService->componentManager()->addComponent<Component>(entity);

			auto& query = *m_query;
			for (auto& updateCountComponent : query)
			{
				++updateCountComponent.count;
			}
		}
	};

	class WriteComponentSystem : public SystemBase
	{
	private:
		Query1<Component>* m_query{};
		std::type_index m_type = std::type_index(typeid(Component));

		Query1<UpdateCountComponent>* m_updateQuery{};

	public:
		void onInitialize() override
		{
			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			m_query = queryBuilder->buildQuery<Component>(buildInfo);

			buildInfo = queryBuilder->getQueryBuildInfo();
			m_updateQuery = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);
		}

		void onUpdate(float deltaTime) override
		{
			sizet querySize = m_query->getSize();


			auto& updateQuery = *m_updateQuery;
			sizet updateIteration = 0;

			for (const auto& updateCountComponent : updateQuery)
			{
				updateIteration = updateCountComponent.count;
			}

			//new entity with component is added in prev system so the size must grow
			EXPECT_EQ(querySize, updateIteration);

			auto& query = *m_query;
			for (auto& componentA : query)
			{
				componentA.data = updateIteration;
			}
		}
	};

	class ReadComponentSystem : public SystemBase
	{
	private:
		Query1<Component>* m_query{};
		std::type_index m_type = std::type_index(typeid(Component));


		Query1<UpdateCountComponent>* m_updateQuery{};

	public:
		void onInitialize() override
		{
			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			m_query = queryBuilder->buildQuery<Component>(buildInfo);

			buildInfo = queryBuilder->getQueryBuildInfo();
			m_updateQuery = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);
		}

		void onUpdate(float deltaTime) override
		{
			auto& updateQuery = *m_updateQuery;
			sizet updateIteration = 0;

			for (const auto& updateCountComponent : updateQuery)
			{
				updateIteration = updateCountComponent.count;
			}

			auto& query = *m_query;
			for (auto& componentA : query)
			{
				EXPECT_EQ(componentA.data, updateIteration);
			}

			++updateIteration;
		}
	};
}

namespace test3
{
	using namespace spite;

	struct Component : IComponent
	{
		sizet data;

		Component() : IComponent() { data = 0; }

		Component(const Component& other)
			: IComponent(other),
			  data(other.data)
		{
		}

		Component(Component&& other) noexcept
			: IComponent(std::move(other)),
			  data(other.data)
		{
		}

		Component& operator=(const Component& other)
		{
			if (this == &other)
				return *this;
			IComponent::operator =(other);
			data = other.data;
			return *this;
		}

		Component& operator=(Component&& other) noexcept
		{
			if (this == &other)
				return *this;

			data = other.data;
			IComponent::operator =(std::move(other));
			return *this;
		}

		~Component() override = default;
	};

	struct UpdateCountComponent : IComponent
	{
		sizet count;

		UpdateCountComponent(): count(0)
		{
		}

		UpdateCountComponent(const UpdateCountComponent& other)
			: IComponent(other),
			  count(other.count)
		{
		}

		UpdateCountComponent(UpdateCountComponent&& other) noexcept
			: IComponent(std::move(other)),
			  count(other.count)
		{
		}

		UpdateCountComponent& operator=(const UpdateCountComponent& other)
		{
			if (this == &other)
				return *this;
			IComponent::operator =(other);
			count = other.count;
			return *this;
		}

		UpdateCountComponent& operator=(UpdateCountComponent&& other) noexcept
		{
			if (this == &other)
				return *this;

			count = other.count;
			IComponent::operator =(std::move(other));
			return *this;
		}

		~UpdateCountComponent() override = default;
	};

	class CreateComponentSystem : public SystemBase
	{
		Query1<UpdateCountComponent>* m_query{};

	public:
		void onInitialize() override
		{
			Entity entity = m_entityService->entityManager()->createEntity();
			m_entityService->componentManager()->addComponent<UpdateCountComponent>(entity);

			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			m_query = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);
		}

		void onUpdate(float deltaTime) override
		{
			Entity entity = m_entityService->entityManager()->createEntity();
			m_entityService->componentManager()->addComponent<Component>(entity);

			auto& query = *m_query;
			for (auto& updateCountComponent : query)
			{
				++updateCountComponent.count;
			}
		}
	};

	class WriteComponentSystem : public SystemBase
	{
	private:
		Query1<Component>* m_query{};
		std::type_index m_type = std::type_index(typeid(Component));

		Query1<UpdateCountComponent>* m_updateQuery{};

	public:
		void onInitialize() override
		{
			requireComponent(m_type);

			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			buildInfo.hasComponent(m_type);
			m_query = queryBuilder->buildQuery<Component>(buildInfo);

			buildInfo = queryBuilder->getQueryBuildInfo();
			m_updateQuery = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);
		}

		void onUpdate(float deltaTime) override
		{
			sizet querySize = m_query->getSize();


			auto& updateQuery = *m_updateQuery;
			sizet updateIteration = 0;

			for (const auto& updateCountComponent : updateQuery)
			{
				updateIteration = updateCountComponent.count;
			}

			//new entity with component is added in prev system so the size must grow
			EXPECT_EQ(querySize, updateIteration);

			auto& query = *m_query;
			for (auto& componentA : query)
			{
				componentA.data = updateIteration;
				//componentA.isActive = false;
			}
		}
	};

	class ReadComponentSystem : public SystemBase
	{
	private:
		Query1<Component>* m_query{};
		std::type_index m_type = std::type_index(typeid(Component));


		Query1<UpdateCountComponent>* m_updateQuery{};

	public:
		void onInitialize() override
		{
			requireComponent(m_type);

			auto queryBuilder = m_entityService->queryBuilder();

			auto buildInfo = queryBuilder->getQueryBuildInfo();
			m_query = queryBuilder->buildQuery<Component>(buildInfo);

			buildInfo = queryBuilder->getQueryBuildInfo();
			m_updateQuery = queryBuilder->buildQuery<UpdateCountComponent>(buildInfo);
		}

		sizet querySize = 0;

		void onUpdate(float deltaTime) override
		{
			auto& updateQuery = *m_updateQuery;
			sizet updateIteration = 0;

			for (const auto& updateCountComponent : updateQuery)
			{
				updateIteration = updateCountComponent.count;
			}

			auto& query = *m_query;
			for (auto& componentA : query.excludeInactive())
			{
				++querySize;
				EXPECT_EQ(componentA.data, updateIteration);
			}
			EXPECT_EQ(querySize, 0);
			++updateIteration;
		}
	};
}
