#include "CameraMatricesUpdateSystem.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/RenderingComponents.hpp"
#include "engine/rendering/NamedBufferRegistry.hpp"
#include "engine/rendering/IRenderer.hpp"

namespace spite
{
	void CameraMatricesUpdateSystem::onUpdate(SystemContext ctx)
	{
		glm::mat4 viewProjection;

		ctx.accessSingleton<CameraMatricesSingleton>([&viewProjection](CameraMatricesSingleton& singleton)
		{
			singleton.view = glm::lookAt(glm::vec3(0.f), glm::vec3(0.f), glm::vec3{0.f, 1.f, 0.f});
			singleton.projection = glm::perspective(90.f, 4.f / 3.f, 1.f, 1000.f);

			viewProjection = singleton.view * singleton.projection;
		});

		ctx.accessSingleton<RendererSingleton>([&viewProjection](const RendererSingleton& singleton)
		{
			auto& registry = singleton.renderer->getNamedBufferRegistry();
			registry.updateBuffer("cameraUBO", &viewProjection, sizeof(viewProjection));
		});
	}
}
