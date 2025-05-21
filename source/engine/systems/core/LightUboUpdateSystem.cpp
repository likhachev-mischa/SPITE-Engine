#include "CoreSystems.hpp"

#include "engine/VulkanLighting.hpp"

namespace spite
{
	void LightUboUpdateSystem::onInitialize()
	{
		m_pointLightQuery = m_entityService->queryBuilder()->buildQuery<
			PointLightComponent, TransformComponent>();
		m_directionalLightQuery = m_entityService->queryBuilder()->buildQuery<
			DirectionalLightComponent, TransformComponent>();
		m_spotlightQuery = m_entityService->queryBuilder()->buildQuery<
			SpotlightComponent, TransformComponent>();
	}

	void LightUboUpdateSystem::onUpdate(float deltaTime)
	{

		//std::vector<PointLightData> lightDatas;
		//lightDatas.reserve(pointLightQuery.size());

		CombinedLightData lightData;

		u32 currentFrame = m_entityService->componentManager()->getSingleton<FrameDataComponent>().
		                                    currentFrame;

		auto& pointLightQuery = *m_pointLightQuery;
		for (sizet i = 0, size = pointLightQuery.size(); i < size; ++i)
		{
			const auto& pointLightData = pointLightQuery.componentT1(i);
			const auto& transform = pointLightQuery.componentT2(i);
			//lightDatas.emplace_back(transform.position, pointLightData.color, pointLightData.intensity, pointLightData.radius);
			lightData.pointLight = PointLightData(transform.position,
			                                      pointLightData.color,
			                                      pointLightData.intensity,
			                                      pointLightData.radius);
		}

		auto& dirLightQuery = *m_directionalLightQuery;
		for (sizet i = 0, size = dirLightQuery.size(); i < size; ++i)
		{
			const auto& dirLightData = dirLightQuery.componentT1(i);
			const auto& transform = dirLightQuery.componentT2(i);

			glm::vec3 lookDirection = transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
			lightData.directionalLight = DirectionalLightData(lookDirection,
				dirLightData.color,dirLightData.intensity);
		}

		auto& spotlightQuery = *m_spotlightQuery;
		for (sizet i = 0, size = spotlightQuery.size(); i < size; ++i)
		{
			const auto& spotlightData = spotlightQuery.componentT1(i);
			const auto& transform = spotlightQuery.componentT2(i);

			glm::vec3 lookDirection = transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
			lightData.spotlight = SpotlightData(transform.position,lookDirection,
				spotlightData.color,spotlightData.intensity,spotlightData.radius,spotlightData.cutoffs);
		}


		Entity lightEntity = m_entityService->entityManager()->getNamedEntity("LightUbo");
		auto& lightUbo = m_entityService->componentManager()->getComponent<
			UniformBufferSharedComponent>(lightEntity);
		memcpy(lightUbo.ubos[currentFrame].mappedMemory, &lightData, lightUbo.elementSize);
	}
}
