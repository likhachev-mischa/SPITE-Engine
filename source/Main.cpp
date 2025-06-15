#include <application/WindowManager.hpp>

#include "application/EventDispatcher.hpp"
#include "application/Time.hpp"
#include "application/input/InputActionMap.hpp"
#include "application/input/InputManager.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/VulkanComponents.hpp"
#include "engine/systems/core/CoreSystems.hpp"
#include "engine/systems/MovementSystems.hpp"
#include "base/memory/AllocatorRegistry.hpp"
#include "external/tracy/tracy/Tracy.hpp"
#include "base/memory/ScratchAllocator.hpp"


int main(int argc, char* argv[])
{
	using namespace spite;
	tracy::InitCallstack();
	spite::initGlobalAllocator();
	spite::FrameScratchAllocator::init();
	spite::AllocatorRegistry::instance().createSubsystemAllocators();
	{
		spite::HeapAllocator allocator = spite::AllocatorRegistry::instance().getAllocator(
			"MainAllocator");

		std::shared_ptr<InputActionMap> inputActionMap = std::make_shared<InputActionMap>(
			"./config/InputActions.json",
			allocator);
		std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>(inputActionMap);
		std::shared_ptr<WindowManager> windowManager = std::make_shared<WindowManager>();
		std::shared_ptr<EventDispatcher> eventDispatcher = std::make_shared<EventDispatcher>(
			inputManager,
			windowManager);

		EntityWorld world(allocator, allocator);
		InputManagerComponent inputManagerComponent;
		inputManagerComponent.inputManager = inputManager;
		world.service()->componentManager()->createSingleton(std::move(inputManagerComponent));
		WindowManagerComponent windowManagerComponent;
		windowManagerComponent.windowManager = windowManager;
		world.service()->componentManager()->createSingleton(std::move(windowManagerComponent));

		std::vector<SystemBase*> systems = {
			new VulkanInitSystem, new LightUboCreateSystem, new CameraCreateSystem,
			new ModelLoadSystem, new TextureLoadSystem,
			//new ShaderCreateSystem,
			//new GeometryPipelineCreateSystem,

			new ControllableEntitySelectSystem, new MovementDirectionControlSystem,
			new RotationControlSystem, new MovementDirectionRotationSynchSystem, new MovementSystem,
			new RotationSystem, new TransformationMatrixSystem, new CameraMatricesUpdateSystem,
			new LightUboUpdateSystem, new CameraUboUpdateSystem, new WaitForFrameSystem,
			new DescriptorUpdateSystem, new DepthPassSystem, new GeometryPassSystem,
			new LightPassSystem, new PresentationSystem, new CleanupSystem,
		};

		world.addSystems(systems.data(), systems.size());

		Entity pointLightEntity = world.service()->entityManager()->createEntity();
		TransformComponent pointLightTransform;
		pointLightTransform.position = {0.5, 0.5, 0};
		world.service()->componentManager()->addComponent(pointLightEntity, pointLightTransform);
		PointLightComponent pointLightComponent;
		pointLightComponent.color = glm::vec3(0.0f, 1.0f, 0.0f);
		pointLightComponent.intensity = 1.0f;
		pointLightComponent.radius = 0.f;
		world.service()->componentManager()->addComponent(pointLightEntity, pointLightComponent);
		ModelLoadRequest pointLightModelReq;
		pointLightModelReq.entity = pointLightEntity;
		pointLightModelReq.objFilePath = "./models/sphere.obj";
		world.service()->entityEventManager()->createEvent(std::move(pointLightModelReq));
		TextureLoadRequest pointLightTextReq;
		pointLightTextReq.path = "./textures/glass.jpg";
		pointLightTextReq.targets = {pointLightEntity};
		world.service()->entityEventManager()->createEvent(std::move(pointLightTextReq));
		SDEBUG_LOG("ENTITY %llu IS POINT LIGHT\n", pointLightEntity.id());

		Entity dirLightEntity = world.service()->entityManager()->createEntity();
		TransformComponent dirLightTransform;
		dirLightTransform.position = {1, 1, 0};
		world.service()->componentManager()->addComponent(dirLightEntity, dirLightTransform);
		DirectionalLightComponent dirLightComponent;
		dirLightComponent.color = glm::vec3(1.0f, 0.0f, 0.0f);
		dirLightComponent.intensity = 1.0f;
		world.service()->componentManager()->addComponent(dirLightEntity, dirLightComponent);
		ModelLoadRequest dirLightModelReq;
		dirLightModelReq.entity = dirLightEntity;
		dirLightModelReq.objFilePath = "./models/sphere.obj";
		world.service()->entityEventManager()->createEvent(std::move(dirLightModelReq));
		TextureLoadRequest dirLightTextReq;
		dirLightTextReq.path = "./textures/wood.jpg";
		dirLightTextReq.targets = {dirLightEntity};
		world.service()->entityEventManager()->createEvent(std::move(dirLightTextReq));
		SDEBUG_LOG("ENTITY %llu IS DIR LIGHT\n", dirLightEntity.id());

		Entity spotlightEntity = world.service()->entityManager()->createEntity();
		TransformComponent spotlightTransform;
		spotlightTransform.position = {2, 1, 0};
		world.service()->componentManager()->addComponent(spotlightEntity, spotlightTransform);
		SpotlightComponent spotlightComponent;
		spotlightComponent.color = glm::vec3(0.0f, 0.0f, 1.0f);
		spotlightComponent.intensity = 100.0f;
		spotlightComponent.radius = 10.0f;
		float innerConeAngleRadians = glm::radians(12.5f);
		float outerConeAngleRadians = glm::radians(17.5f);
		spotlightComponent.cutoffs = glm::vec2(glm::cos(innerConeAngleRadians),
		                                       glm::cos(outerConeAngleRadians));
		world.service()->componentManager()->addComponent(spotlightEntity, spotlightComponent);
		ModelLoadRequest spotlightModelReq;
		spotlightModelReq.entity = spotlightEntity;
		spotlightModelReq.objFilePath = "./models/sphere.obj";
		world.service()->entityEventManager()->createEvent(std::move(spotlightModelReq));
		TextureLoadRequest spotlightTextReq;
		spotlightTextReq.path = "./textures/glass.jpg";
		spotlightTextReq.targets = {spotlightEntity};
		world.service()->entityEventManager()->createEvent(std::move(spotlightTextReq));
		SDEBUG_LOG("ENTITY %llu IS SPOTLIGHT\n", spotlightEntity.id());
#ifdef SPITE_TEST
		SDEBUG_LOG("TEST DEFINED!!!!!!!\n");
#endif


		constexpr sizet ENTITIES_SIZE = 10;
		std::vector<Entity> entities;
		entities.reserve(ENTITIES_SIZE * ENTITIES_SIZE);

		const float spacing = 5.0f;
		// Y-coordinate for all entities in the grid
		const float y_coordinate = 0.0f;

		// Calculate an offset to center the grid around the origin (0, y_coordinate, 0)
		// This offset applies to both x and z axes.
		const float grid_center_offset = (static_cast<float>(ENTITIES_SIZE) - 1.0f) * spacing /
			2.0f;

		auto cbuffer = world.service()->getCommandBuffer<TransformComponent>();
		cbuffer.reserveForAddition(ENTITIES_SIZE * ENTITIES_SIZE);
		auto tagCbuffer = world.service()->getCommandBuffer<RotationTargetTag>();
		tagCbuffer.reserveForAddition(ENTITIES_SIZE * ENTITIES_SIZE);

		for (int ix = 0; ix < ENTITIES_SIZE; ++ix)
		{
			for (int iz = 0; iz < ENTITIES_SIZE; ++iz)
			{
				// Create a new entity
				Entity entity = world.service()->entityManager()->createEntity();;
				entities.push_back(entity);

				// Add the Model component
				// 'shared_model_asset' should be your pre-loaded model data/handle
				ModelLoadRequest cubeModelLoadReq;
				cubeModelLoadReq.objFilePath = "./models/cube2.obj";
				cubeModelLoadReq.entity = entity;
				world.service()->entityEventManager()->createEvent(std::move(cubeModelLoadReq));
				//m_Registry.emplace<Model>(entity, shared_model_asset);

				// Create and configure the Transform component
				TransformComponent transform;
				// Calculate position for the current entity in the grid
				transform.position.x = static_cast<float>(ix) * spacing - grid_center_offset;
				transform.position.y = y_coordinate;
				transform.position.z = static_cast<float>(iz) * spacing - grid_center_offset;
				//world.service()->componentManager()->addComponent(entity,std::move(transform));
				cbuffer.addComponent(entity, std::move(transform));
				tagCbuffer.addComponent(entity, {});

				// Optional: Set default rotation (e.g., identity) and scale (e.g., 1.0)
				// transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
				// transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);    // Default scale

				// Add the Transform component to the entity
				//m_Registry.emplace<Transform>(entity, transform);
			}
		}
		cbuffer.commit();
		tagCbuffer.commit();

		TextureLoadRequest textureLoadRequest;
		textureLoadRequest.path = "./textures/wood.jpg";
		textureLoadRequest.targets.resize(entities.size());
		std::copy_n(entities.begin(), entities.size(), textureLoadRequest.targets.begin());
		world.service()->entityEventManager()->createEvent(std::move(textureLoadRequest));


		//for (sizet i = 0; i < ENTITIES_SIZE; ++i)
		//{
		//	entity =
		//	TransformComponent cubeTransform;
		//	world.service()->componentManager()->addComponent(entity, cubeTransform);
		//	ModelLoadRequest cubeModelLoadReq;
		//	cubeModelLoadReq.objFilePath = "./models/cube.obj";
		//	cubeModelLoadReq.entity = entity;
		//	world.service()->entityEventManager()->createEvent(std::move(cubeModelLoadReq));

		//	//TextureLoadRequest cubeTextLoadReq;
		//	//cubeTextLoadReq.path = "./textures/wood.jpg";
		//	//cubeTextLoadReq.targets = {cubeEntity};
		//	//world.service()->entityEventManager()->createEvent(std::move(cubeTextLoadReq));
		//}

		world.commitSystemsStructuralChange();
		world.start();
		world.enable();
		//world.commitSystemsStructuralChange();

		Time time;
		while (!windowManager->shouldTerminate())
		{
			inputManager->update(Time::deltaTime());
			eventDispatcher->pollEvents();
			world.update(Time::deltaTime());
			world.commitSystemsStructuralChange();
			time.updateDeltaTime();
			//TODO: HOLDING TIME SHOULD UPDATE IN LOOP, INSTEAD OF RELYING ON EXTERNAL EVENTS
			inputManager->reset();
			FrameScratchAllocator::get().print_stats();
			FrameScratchAllocator::resetFrame();
			FrameMark;
		}
		CleanupRequest cleanupRequest{};
		world.service()->entityEventManager()->createEvent(cleanupRequest);
		world.commitSystemsStructuralChange();
		world.update(0.1f);
	}

	spite::FrameScratchAllocator::shutdown();
	spite::AllocatorRegistry::instance().shutdownAll();
	SDEBUG_LOG("\n===SHUTTING DOWN GLOBAL ALLOCATOR===\n")
	spite::shutdownGlobalAllocator(false);
}
