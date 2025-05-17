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
		new VulkanInitSystem,
		new CameraCreateSystem,
		new ModelLoadSystem,
		new ShaderCreateSystem,
		new GeometryPipelineCreateSystem,

		new MovementDirectionControlSystem,
		new RotationControlSystem,
		new MovementDirectionRotationSynchSystem,
		new MovementSystem,

		new TransformationMatrixSystem,
		new CameraMatricesUpdateSystem,
		new CameraUboUpdateSystem,

		new WaitForFrameSystem,
		new DescriptorUpdateSystem,
		
		new DepthPassSystem,
		new GeometryPassSystem,
		new LightPassSystem,

		new PresentationSystem,

		new CleanupSystem,
	};

	world.addSystems(systems.data(), systems.size());

	Entity entity1 = world.service()->entityManager()->createEntity();
	TransformComponent transform1;
	transform1.position = {0.5, 0.5, 0};
	world.service()->componentManager()->addComponent(entity1, transform1);

	ModelLoadRequest modelLoadRequest;
	modelLoadRequest.entity = entity1;
	modelLoadRequest.objFilePath = "./models/cube.obj";
	modelLoadRequest.vertShaderPath = "./shaders/geometryVert.spv";
	modelLoadRequest.fragShaderPath = "./shaders/geometryFrag.spv";
	world.service()->entityEventManager()->createEvent(std::move(modelLoadRequest));

	ModelLoadRequest modelLoadRequest2;
	modelLoadRequest2.objFilePath = "./models/cube.obj";
	modelLoadRequest2.vertShaderPath = "./shaders/geometryVert.spv";
	modelLoadRequest2.fragShaderPath = "./shaders/geometryFrag.spv";
	world.service()->entityEventManager()->createEvent(std::move(modelLoadRequest2));

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
