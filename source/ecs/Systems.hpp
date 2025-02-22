#pragma once
#include "ecs/Core.hpp"
#include "ecs/World.hpp"

namespace spite
{
	//struct FragmentData;
	//class HeapAllocator;

	//void updateTransformMatricesSystem(const eastl::vector<Transform, spite::HeapAllocator>& transforms,
	//                                   eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices);

	//void updateCameraSystem(const CameraData& cameraData,
	//                        const eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices,
	//                        const u16 cameraIdx, CameraMatrices& cameraMatrices);

	//void updateTransformUboSystem(void* memory, sizet elementAlignment,
	//                              const eastl::vector<TransformMatrix, spite::HeapAllocator>& transformMatrices);

	//void updateCameraUboSystem(void* memory, const CameraMatrices& matrices);
	//void updateFragUboSystem(void* memory, sizet elementAlignment,
	//                         const eastl::vector<FragmentData, spite::HeapAllocator>& fragmentDatas);


	//struct ComponentA : IComponent
	//{
	//	int data;
	//	ComponentA() : IComponent() { data = 0; }

	//	ComponentA(ComponentA&& other) noexcept
	//		: IComponent(other), data(other.data)
	//	{
	//	}

	//	ComponentA(const ComponentA& other)
	//		: IComponent(other), data(other.data)
	//	{
	//	}

	//	ComponentA& operator=(ComponentA&& other) noexcept
	//	{
	//		if (this == &other)
	//			return *this;
	//		IComponent::operator=(other);
	//		data = other.data;
	//		return *this;
	//	}
	//};

	//struct ComponentB : IComponent
	//{
	//	int data;
	//	ComponentB() : IComponent() { data = 0; }

	//	ComponentB(const ComponentB& other)
	//		: IComponent(other),
	//		  data(other.data)
	//	{
	//	}

	//	ComponentB(ComponentB&& other) noexcept
	//		: IComponent(std::move(other)),
	//		  data(other.data)
	//	{
	//	}

	//	ComponentB& operator=(const ComponentB& other)
	//	{
	//		if (this == &other)
	//			return *this;
	//		IComponent::operator =(other);
	//		data = other.data;
	//		return *this;
	//	}

	//	ComponentB& operator=(ComponentB&& other) noexcept
	//	{
	//		if (this == &other)
	//			return *this;
	//		IComponent::operator =(std::move(other));
	//		data = other.data;
	//		return *this;
	//	}
	//};

	//struct ComponentC : IComponent
	//{
	//	int data;
	//	ComponentC() : IComponent() { data = 0; }

	//	ComponentC(const ComponentC& other)
	//		: IComponent(other),
	//		  data(other.data)
	//	{
	//	}

	//	ComponentC(ComponentC&& other) noexcept
	//		: IComponent(std::move(other)),
	//		  data(other.data)
	//	{
	//	}

	//	ComponentC& operator=(const ComponentC& other)
	//	{
	//		if (this == &other)
	//			return *this;
	//		IComponent::operator =(other);
	//		data = other.data;
	//		return *this;
	//	}

	//	ComponentC& operator=(ComponentC&& other) noexcept
	//	{
	//		if (this == &other)
	//			return *this;
	//		IComponent::operator =(std::move(other));
	//		data = other.data;
	//		return *this;
	//	}
	//};

	//struct ComponentD : IComponent
	//{
	//	int data;
	//	ComponentD() : IComponent() { data = 0; }

	//	ComponentD(const ComponentD& other)
	//		: IComponent(other),
	//		  data(other.data)
	//	{
	//	}

	//	ComponentD(ComponentD&& other) noexcept
	//		: IComponent(std::move(other)),
	//		  data(other.data)
	//	{
	//	}

	//	ComponentD& operator=(const ComponentD& other)
	//	{
	//		if (this == &other)
	//			return *this;
	//		IComponent::operator =(other);
	//		data = other.data;
	//		return *this;
	//	}

	//	ComponentD& operator=(ComponentD&& other) noexcept
	//	{
	//		if (this == &other)
	//			return *this;
	//		IComponent::operator =(std::move(other));
	//		data = other.data;
	//		return *this;
	//	}
	//};


	//class TestSystem1 : public SystemBase
	//{
	//public:
	//	void onInitialize() override
	//	{
	//		Entity entity = m_entityService->entityManager()->createEntity();
	//		m_entityService->componentManager()->addComponent<ComponentA>(entity);
	//		m_entityService->componentManager()->addComponent<ComponentD>(entity);

	//		Entity entity2 = m_entityService->entityManager()->createEntity();
	//		m_entityService->componentManager()->addComponent<ComponentA>(entity2);
	//		m_entityService->componentManager()->addComponent<ComponentB>(entity2);
	//		m_entityService->componentManager()->addComponent<ComponentC>(entity2);

	//		Entity entity3 = m_entityService->entityManager()->createEntity();
	//		m_entityService->componentManager()->addComponent<ComponentA>(entity3);
	//		m_entityService->componentManager()->addComponent<ComponentD>(entity3);

	//		Entity entity4 = m_entityService->entityManager()->createEntity();
	//		m_entityService->componentManager()->addComponent<ComponentA>(entity4);
	//		m_entityService->componentManager()->addComponent<ComponentD>(entity4);

	//		Entity entity5 = m_entityService->entityManager()->createEntity();
	//		m_entityService->componentManager()->addComponent<ComponentA>(entity5);
	//		m_entityService->componentManager()->addComponent<ComponentD>(entity5);
	//	}

	//	void onUpdate(float deltaTime) override
	//	{
	//		Entity entity = m_entityService->entityManager()->createEntity();
	//		m_entityService->componentManager()->addComponent<ComponentA>(entity);
	//		m_entityService->componentManager()->addComponent<ComponentD>(entity);

	//		Entity entity2 = m_entityService->entityManager()->createEntity();
	//		m_entityService->componentManager()->addComponent<ComponentA>(entity2);
	//		m_entityService->componentManager()->addComponent<ComponentB>(entity2);
	//		m_entityService->componentManager()->addComponent<ComponentD>(entity2);
	//	}
	//};

	//class TestSystem2 : public SystemBase
	//{
	//	Query1<ComponentA>* m_query{};

	//	Query1<ComponentD>* m_query2{};

	//public:
	//	void onInitialize() override
	//	{
	//		auto queryInfo = m_entityService->queryBuilder()->getQueryBuildInfo();
	//		queryInfo.hasComponent(std::type_index(typeid(ComponentB))).
	//		          hasComponent(std::type_index(typeid(ComponentC))).
	//		          hasNoComponent(std::type_index(typeid(ComponentD)));

	//		m_query = m_entityService->queryBuilder()->buildQuery<ComponentA>(queryInfo);

	//		auto queryInfo2 = m_entityService->queryBuilder()->getQueryBuildInfo();

	//		queryInfo2.hasComponent(std::type_index(typeid(ComponentD)));
	//		m_query2 = m_entityService->queryBuilder()->buildQuery<ComponentD>(queryInfo2);
	//	}

	//	void onUpdate(float deltaTime) override
	//	{
	//		for (sizet i = 0, size = m_query->getSize(); i < size; ++i)
	//		{
	//			SDEBUG_LOG("data is %i, owner is %llu\n", m_query->operator[](i).
	//			           data, m_query->operator[](i).owner.id())
	//		}

	//		auto cbuf = m_entityService->getCommandBuffer<ComponentD>();
	//		SDEBUG_LOG("\nCOMPONENT D SIZE %llu\n\n", m_query2->getSize());
	//		for (sizet i = 0, size = m_query2->getSize(); i < size; ++i)
	//		{
	//			SDEBUG_LOG("componentD %llu owner\n", m_query2->operator[](i).owner.id());
	//			cbuf.removeComponent(m_query2->operator[](i).owner);
	//		}
	//		cbuf.commit();
	//	}
	//};
}
