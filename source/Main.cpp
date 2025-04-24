#include <application/WindowManager.hpp>

#include "application/EventDispatcher.hpp"
#include "application/Time.hpp"
#include "application/input/InputActionMap.hpp"
#include "application/input/InputManager.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/VulkanComponents.hpp"
#include "engine/systems/core/CoreSystems.hpp"


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
		new VulkanInitSystem, new CameraCreateSystem, new ModelLoadSystem, new ShaderCreateSystem,
		new PipelineCreateSystem, new TransformationMatrixSystem, new CameraMatricesUpdateSystem,
		new CameraUboUpdateSystem, new WaitForFrameSystem, new DescriptorUpdateSystem,
		new RenderSystem, new CleanupSystem,
	};

	world.addSystems(systems.data(), systems.size());

	Entity entity1 = world.service()->entityManager()->createEntity();
	TransformComponent transform1;
	transform1.position = {0.5, 0.5, 0};
	world.service()->componentManager()->addComponent(entity1, transform1);

	ModelLoadRequest modelLoadRequest;
	modelLoadRequest.entity = entity1;
	modelLoadRequest.objFilePath = "./models/cube.obj";
	modelLoadRequest.vertShaderPath = "./shaders/vert.spv";
	modelLoadRequest.fragShaderPath = "./shaders/frag.spv";
	world.service()->entityEventManager()->createEvent(std::move(modelLoadRequest));

	ModelLoadRequest modelLoadRequest2;
	modelLoadRequest2.objFilePath = "./models/cube.obj";
	modelLoadRequest2.vertShaderPath = "./shaders/vert.spv";
	modelLoadRequest2.fragShaderPath = "./shaders/frag.spv";
	world.service()->entityEventManager()->createEvent(std::move(modelLoadRequest2));

	world.commitSystemsStructuralChange();
	world.enable();

	Time time;
	while (!windowManager->shouldTerminate())
	{
		eventDispatcher->pollEvents();
		world.update(Time::deltaTime());
		world.commitSystemsStructuralChange();
		time.updateDeltaTime();
		inputManager->reset();
	}

	CleanupRequest cleanupRequest{};
	world.service()->entityEventManager()->createEvent(cleanupRequest);
	world.commitSystemsStructuralChange();
	world.update(0.1f);
}
