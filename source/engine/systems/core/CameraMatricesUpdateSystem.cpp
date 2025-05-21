#include "CoreSystems.hpp"

namespace spite
{
	void CameraMatricesUpdateSystem::onInitialize()
	{
		m_query1 = m_entityService->queryBuilder()->buildQuery<
			TransformComponent, CameraMatricesComponent>();
		m_query2 = m_entityService->queryBuilder()->buildQuery<
			CameraDataComponent, CameraMatricesComponent>();
	}

	void CameraMatricesUpdateSystem::onUpdate(float deltaTime)
	{
		auto& query1 = *m_query1;

		for (sizet i = 0, size = query1.size(); i < size; ++i)
		{
			TransformComponent& transform = query1.componentT1(i);
			CameraMatricesComponent& matrices = query1.componentT2(i);

			matrices.view = createViewMatrix(transform.position, transform.rotation);
			//matrices.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
			//                           glm::vec3(0.0f, 0.0f, 0.0f),
			//                          glm::vec3(0.0f, 0.0f, 1.0f));
		}

		auto& query2 = *m_query2;

		//TODO: store aspect in swapchain

		auto& swapchainComponent = m_entityService->componentManager()->getSingleton<
			SwapchainComponent>();
		float aspect = swapchainComponent.extent.width / static_cast<float>(swapchainComponent.
			extent.height);
		for (sizet i = 0, size = query2.size(); i < size; ++i)
		{
			CameraDataComponent& data = query2.componentT1(i);
			//update projection matrix only when neccesary
			if (!data.isDirty)
			{
				continue;
			}
			CameraMatricesComponent& matrices = query2.componentT2(i);
			matrices.projection = createProjectionMatrix(data, aspect);
		}
	}

	glm::mat4 CameraMatricesUpdateSystem::createViewMatrix(const glm::vec3& position,
	                                                       const glm::quat& rotation)
	{
		glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);

		glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

		glm::vec3 target = position + forward;

		return glm::lookAt(position, target, up);
	}

	glm::mat4 CameraMatricesUpdateSystem::createProjectionMatrix(CameraDataComponent& data,
	                                                             float aspect)
	{
		data.isDirty = false;

		glm::mat4 proj = glm::perspective(data.fov, aspect, data.nearPlane, data.farPlane);
		proj[1][1] *= -1;
		return proj;
	}
}
