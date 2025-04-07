#include <application/WindowManager.hpp>
#include <engine/VulkanSystems.hpp>

#include "application/EventManager.hpp"
#include "application/InputManager.hpp"


int main(int argc, char* argv[])
{
	using namespace spite;

	std::shared_ptr<EventManager> eventManager = std::make_shared<EventManager>();
	std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>();
	std::shared_ptr<WindowManager> windowManager = std::make_shared<WindowManager>(
		eventManager,
		inputManager);

	HeapAllocator allocator("MAIN ALLOCATOR");

	EntityWorld world(allocator, allocator);

	WindowManagerComponent windowManagerComponent;
	windowManagerComponent.windowManager = windowManager;
	world.service()->componentManager()->createSingleton(windowManagerComponent);

	std::vector<SystemBase*> systems = {
		new VulkanInitSystem,
		new CameraCreateSystem,
		new ModelLoadSystem,
		new ShaderCreateSystem,
		new PipelineCreateSystem,
		new TransformationMatrixSystem,
		new CameraMatricesUpdateSystem,
		new CameraUboUpdateSystem,
		new WaitForFrameSystem,
		new DescriptorUpdateSystem,
		new RenderSystem,
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

	while (!windowManager->shouldTerminate())
	{
		windowManager->pollEvents();
		world.update(0.1f);
		world.commitSystemsStructuralChange();
	}

	CleanupRequest cleanupRequest{};
	world.service()->entityEventManager()->createEvent(cleanupRequest);
	world.commitSystemsStructuralChange();
	world.update(0.1f);
}
