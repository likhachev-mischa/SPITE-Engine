#include <application/WindowManager.hpp>

#include "application/EventDispatcher.hpp"
#include "application/Time.hpp"
#include "application/input/InputActionMap.hpp"
#include "application/input/InputManager.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/VulkanComponents.hpp"
#include "engine/systems/core/CoreSystems.hpp"
#include "engine/systems/MovementSystems.hpp"


int main(int argc, char* argv[])
{
	using namespace spite;

	HeapAllocator allocator("MAIN ALLOCATOR");

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
	world.service()->componentManager()->createSingleton(inputManagerComponent);

	WindowManagerComponent windowManagerComponent;
	windowManagerComponent.windowManager = windowManager;
	world.service()->componentManager()->createSingleton(windowManagerComponent);


	std::vector<SystemBase*> systems = {
		new VulkanInitSystem, new LightUboCreateSystem, new CameraCreateSystem, new ModelLoadSystem,
		new TextureLoadSystem,
		//new ShaderCreateSystem,
		//new GeometryPipelineCreateSystem,

		new ControllableEntitySelectSystem, new MovementDirectionControlSystem,
		new RotationControlSystem, new MovementDirectionRotationSynchSystem, new MovementSystem,
		new TransformationMatrixSystem, new CameraMatricesUpdateSystem, new LightUboUpdateSystem,
		new CameraUboUpdateSystem, new WaitForFrameSystem, new DescriptorUpdateSystem,
		new DepthPassSystem, new GeometryPassSystem, new LightPassSystem, new PresentationSystem,
		new CleanupSystem,
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
	dirLightTextReq.path = "./textures/glass.jpg";
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
	spotlightComponent.cutoffs = glm::vec2(glm::cos(innerConeAngleRadians), glm::cos(outerConeAngleRadians));
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

	Entity cubeEntity = world.service()->entityManager()->createEntity();
	TransformComponent cubeTransform;
	cubeTransform.scale = glm::vec3(100.f, 100.f, 100.f);
	world.service()->componentManager()->addComponent(cubeEntity, cubeTransform);
	ModelLoadRequest cubeModelLoadReq;
	cubeModelLoadReq.objFilePath = "./models/cornell_box.obj";
	cubeModelLoadReq.entity = cubeEntity;
	world.service()->entityEventManager()->createEvent(std::move(cubeModelLoadReq));

	TextureLoadRequest cubeTextLoadReq;
	cubeTextLoadReq.path = "./textures/box_texture.png";
	cubeTextLoadReq.targets = {cubeEntity};
	world.service()->entityEventManager()->createEvent(std::move(cubeTextLoadReq));

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
	}

	CleanupRequest cleanupRequest{};
	world.service()->entityEventManager()->createEvent(cleanupRequest);
	world.commitSystemsStructuralChange();
	world.update(0.1f);
}
