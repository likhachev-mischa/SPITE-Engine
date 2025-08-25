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

#include "base/StringInterner.hpp"

#include "engine/rendering/IRenderer.hpp"
#include "engine/rendering/RenderingManager.hpp"
#include "engine/systems/BeginFrameSystem.hpp"
#include "engine/systems/CameraMatricesUpdateSystem.hpp"
#include "engine/systems/CompositePassSystem.hpp"
#include "engine/systems/DepthPassSystem.hpp"
#include "engine/systems/EventCleanupSystem.hpp"
#include "engine/systems/GeometryPassSystem.hpp"
#include "engine/systems/LightPassSystem.hpp"
#include "engine/systems/ModelLoadSystem.hpp"
#include "engine/systems/RenderSystem.hpp"
#include "engine/systems/TransformMatrixCalculateSystem.hpp"
#include "engine/ui/ReflectedComponents.hpp"
#include "engine/ui/TypeInspectorRegistry.hpp"
#include "engine/ui/UIInspectorManager.hpp"


int main(int argc, char* argv[])
{
	using namespace spite;
	tracy::InitCallstack();
	spite::initGlobalAllocator();
	StringInterner::init(getGlobalAllocator());
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
			DepthPassSystem, GeometryPassSystem, LightPassSystem, CompositePassSystem, RenderSystem,
			EventCleanupSystem>();


		RenderingManager renderingManager(GraphicsApi::Vulkan, *windowManager, getGlobalAllocator());

		world.getEntityManager().registerSingletonComponent(
			std::make_unique<RenderGraphSingleton>(RenderGraphSingleton{
				.renderGraph = renderingManager.getApiManager().renderGraph()
			}));
		world.getEntityManager().registerSingletonComponent(
			std::make_unique<RenderingManagerSingleton>(
				RenderingManagerSingleton{.renderingManager = &renderingManager}));
		world.getEntityManager().registerSingletonComponent(
			std::make_unique<RendererSingleton>(RendererSingleton{
				.renderer = renderingManager.getApiManager().renderer()
			}));
		world.getEntityManager().registerSingletonComponent(
			std::make_unique<RenderDeviceSingleton>(RenderDeviceSingleton{
				.renderDevice = &renderingManager.getApiManager().renderer()->getDevice()
			}));
		world.getEntityManager().registerSingletonComponent(std::make_unique<RenderResourceManagerSingleton>(
			RenderResourceManagerSingleton{
				.resourceManager = &renderingManager.getApiManager().renderer()->getDevice().getResourceManager()
			}));

		world.initialize();

		UIInspectorManager::init(*windowManager, *renderingManager.getApiManager().renderer(),
		                         world.getEntityManager());

		world.getEntityManager().getEventManager().fire<ModelLoadRequest>(ModelLoadRequest{
			.filePath = toHashedString("models/cube2.obj")
		});

		world.getEntityManager().getEventManager().fire<ModelLoadRequest>(ModelLoadRequest{
			.filePath = toHashedString("models/cube2.obj")
		});

		//TODO clean main
		ReflectionRegistry::init(getGlobalAllocator());
		TypeInspectorRegistry::init(getGlobalAllocator());
		registerReflectedComponents();

		sizet update = 1;
		while (!windowManager->shouldTerminate())
		{
			//SDEBUG_LOG("Frame %zu start\n", update)
			world.getEntityManager().getEventManager().commit();
			eventDispatcher.pollEvents();
			world.update(0.1f);
			FrameScratchAllocator::resetFrame();
			//SDEBUG_LOG("Frame %zu finish\n", update)
			++update;
		}
	}
	ReflectionRegistry::destroy();
	TypeInspectorRegistry::destroy();

	FrameScratchAllocator::shutdown();
	ComponentMetadataRegistry::destroy();
	StringInterner::destroy();
	spite::shutdownGlobalAllocator();
}
