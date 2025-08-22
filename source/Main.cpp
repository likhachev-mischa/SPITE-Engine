#include <external/tracy/client/TracyCallstack.hpp>

#include "application/EventDispatcher.hpp"
#include "application/WindowManager.hpp"
#include "application/input/InputActionMap.hpp"
#include "application/input/InputManager.hpp"

#include "base/memory/HeapAllocator.hpp"

#include "ecs/core/EntityWorld.hpp"
#include "ecs/systems/SystemBase.hpp"

#include "engine/components/RenderingComponents.hpp"
#include <engine/rendering/IRenderDevice.hpp>
#include "engine/rendering/IRenderer.hpp"
#include "engine/rendering/RenderingManager.hpp"
#include "engine/systems/BeginFrameSystem.hpp"
#include "engine/systems/CameraMatricesUpdateSystem.hpp"
#include "engine/systems/DepthPassSystem.hpp"
#include "engine/systems/EventCleanupSystem.hpp"
#include "engine/systems/GeometryPassSystem.hpp"
#include "engine/systems/LightPassSystem.hpp"
#include "engine/systems/ModelLoadSystem.hpp"
#include "engine/systems/RenderSystem.hpp"
#include "engine/systems/TransformMatrixCalculateSystem.hpp"


int main(int argc, char* argv[])
{
	using namespace spite;
	tracy::InitCallstack();
	spite::initGlobalAllocator();
	{
		FrameScratchAllocator::init();
		ComponentMetadataRegistry::init(spite::getGlobalAllocator());

		auto windowManager = std::make_shared<WindowManager>(GraphicsApi::Vulkan);
		auto actionMap = std::make_shared<InputActionMap>("config/InputActions.json", getGlobalAllocator());
		auto inputManager = std::make_shared<InputManager>(actionMap);

		EventDispatcher eventDispatcher(inputManager, windowManager);

		spite::EntityWorld world(spite::getGlobalAllocator());

		world.getSystemManager().registerSystems<
			ModelLoadSystem, CameraMatricesUpdateSystem, TransformMatrixCalculateSystem,
			BeginFrameSystem,
			DepthPassSystem, GeometryPassSystem, LightPassSystem, RenderSystem, EventCleanupSystem>();


		RenderingManager renderer(GraphicsApi::Vulkan, *windowManager, getGlobalAllocator());

		world.getEntityManager().registerSingletonComponent(
			std::make_unique<RenderGraphSingleton>(RenderGraphSingleton{
				.renderGraph = renderer.getApiManager().renderGraph()
			}));
		world.getEntityManager().registerSingletonComponent(
			std::make_unique<RenderingManagerSingleton>(RenderingManagerSingleton{.renderingManager = &renderer}));
		world.getEntityManager().registerSingletonComponent(
			std::make_unique<RendererSingleton>(RendererSingleton{.renderer = renderer.getApiManager().renderer()}));
		world.getEntityManager().registerSingletonComponent(
			std::make_unique<RenderDeviceSingleton>(RenderDeviceSingleton{
				.renderDevice = &renderer.getApiManager().renderer()->getDevice()
			}));
		world.getEntityManager().registerSingletonComponent(std::make_unique<RenderResourceManagerSingleton>(
			RenderResourceManagerSingleton{
				.resourceManager = &renderer.getApiManager().renderer()->getDevice().getResourceManager()
			}));

		world.initialize();

		world.getEntityManager().getEventManager().fire<ModelLoadRequest>(ModelLoadRequest{
			.filePath = "models/cube2.obj"
		});

		sizet update = 1;
		while (!windowManager->shouldTerminate())
		{
			SDEBUG_LOG("Frame %zu start\n", update)
			world.getEntityManager().getEventManager().commit();
			eventDispatcher.pollEvents();
			world.update(0.1f);
			world.getEntityManager().getArchetypeManager()->resetAllModificationTracking();
			FrameScratchAllocator::resetFrame();
			SDEBUG_LOG("Frame %zu finish\n", update)
			++update;
		}
	}
	FrameScratchAllocator::shutdown();
	ComponentMetadataRegistry::destroy();
	spite::shutdownGlobalAllocator();
}
