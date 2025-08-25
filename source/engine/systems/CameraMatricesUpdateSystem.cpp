#include "CameraMatricesUpdateSystem.hpp"

#include "base/StringInterner.hpp"

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
			singleton.view = glm::mat4(1);
			singleton.projection = glm::perspective(90.f, 4.f / 3.f, 1.f, 1000.f);

			viewProjection = singleton.view * singleton.projection;
		});

		ctx.accessSingleton<RendererSingleton>([&viewProjection](const RendererSingleton& singleton)
		{
			auto& registry = singleton.renderer->getNamedBufferRegistry();
			registry.updateBuffer(toHashedString("cameraUBO"), &viewProjection, sizeof(viewProjection));
		});
	}
}
