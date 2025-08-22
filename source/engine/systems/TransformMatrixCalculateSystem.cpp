#include "TransformMatrixCalculateSystem.hpp"

#include "engine/components/CoreComponents.hpp"


namespace spite
{
	void TransformMatrixCalculateSystem::onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage)
	{
		setExecutionStage(CoreExecutionStages::POST_UPDATE);

		auto& queryDesc = ctx.getQueryBuilder().with<Read<TransformComponent>, Write<TransformMatrixComponent>>().
		                      modified<TransformComponent>();

		query = registerQuery(queryDesc, dependencyStorage);
		setPrerequisite(query);
	}

	void TransformMatrixCalculateSystem::onUpdate(SystemContext ctx)
	{
		for (auto [matrix, transform] : query.view<Write<TransformMatrixComponent>, Read<TransformComponent>>())
		{
			matrix.matrix = glm::mat4(1.0f);

			matrix.matrix = glm::translate(matrix.matrix, transform.position);
			matrix.matrix = matrix.matrix * glm::mat4_cast(transform.rotation);
			matrix.matrix = glm::scale(matrix.matrix, transform.scale);
		}
	}
}
