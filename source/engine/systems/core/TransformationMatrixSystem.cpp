#include "CoreSystems.hpp"

namespace spite
{
	void TransformationMatrixSystem::onInitialize()
	{
		m_query = m_entityService->queryBuilder()->buildQuery<
			TransformComponent, TransformMatrixComponent>();
	}

	void TransformationMatrixSystem::onUpdate(float deltaTime)
	{
		auto& query = *m_query;
		for (sizet i = 0, size = query.size(); i < size; ++i)
		{
			auto& transform = query.componentT1(i);
			//only recalculate dirty matrices 
			if (!transform.isDirty)
			{
				continue;
			}

			auto& matrix = query.componentT2(i);
			matrix.matrix = calculateModelMatrix(query.owner(i), transform);
		}
	}

	glm::mat4 TransformationMatrixSystem::calculateModelMatrix(Entity entity,
	                                                           TransformComponent& transform)
	{
		// Calculate local transformation
		glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), transform.position) *
			glm::mat4_cast(transform.rotation) * glm::scale(glm::mat4(1.0f), transform.scale);

		transform.isDirty = false;

		// Apply parent transformation if any
		if (transform.parent != Entity::undefined())
		{
			TransformComponent& parentTransform = m_entityService->componentManager()->getComponent<
				TransformComponent>(transform.parent);

			glm::mat4 parentMatrix;
			if (parentTransform.isDirty)
			{
				parentMatrix = calculateModelMatrix(transform.parent, parentTransform);
			}
			else
			{
				parentMatrix = m_entityService->componentManager()->getComponent<
					TransformMatrixComponent>(transform.parent).matrix;
			}
			return parentMatrix * localMatrix;
		}

		return localMatrix;
	}

	
}
