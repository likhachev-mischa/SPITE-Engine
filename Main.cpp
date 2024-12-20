#include <chrono>
#include <thread>

#include <EASTL/array.h>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include<glm/gtc/quaternion.hpp>

#include "Application/AppConifg.hpp"
#include "Application/EventManager.hpp"
#include "Application/InputManager.hpp"
#include "Application/WindowManager.hpp"

#include "Base/Assert.hpp"
#include "Base/Common.hpp"
#include "Base/File.hpp"
#include "Base/Logging.hpp"
#include "Base/Memory.hpp"

#include "Engine/Modules.hpp"
#include "Engine/ECS/ComponentsCore.hpp"
#include "Engine/ECS/SystemsCore.hpp"


//struct EventContext
//{
//	spite::GraphicsEngine* engine;
//	spite::WindowManager* windowManager;
//	size_t modelCount;
//	size_t currentModel;
//	Transform* transforms;
//};

//void onFrameBufferResized(EventContext& context)
//{
//	context.engine->framebufferResized = true;
//}

/*void onTranslationButtonPressed(EventContext& context)
{
	float x, y;
	std::cout << "Enter translation values x,y: ";
	std::cin >> x >> y;
	std::cout << "\n";
	size_t currentModel = context.currentModel;
	Transform* pCurrentTransform = &context.transforms[currentModel];
	pCurrentTransform->translation = glm::translate(pCurrentTransform->translation, glm::vec3(x, y, 0.f));

	context.engine->setUbo({
		pCurrentTransform->translation * pCurrentTransform->rotation * pCurrentTransform->scale,
		pCurrentTransform->color
	});
}

void onScaleButtonPressed(EventContext& context)
{
	float s;
	std::cout << "Enter scale value: ";
	std::cin >> s;
	std::cout << "\n";
	size_t currentModel = context.currentModel;
	Transform* pCurrentTransform = &context.transforms[currentModel];
	pCurrentTransform->scale = glm::scale(pCurrentTransform->scale, glm::vec3(s, s, 0.f));
	context.engine->setUbo({
		pCurrentTransform->translation * pCurrentTransform->rotation * pCurrentTransform->scale,
		pCurrentTransform->color
	});
}


void onRotationButtonPressed(EventContext& context)
{
	float angle;
	std::cout << "Enter rotation angle: ";
	std::cin >> angle;
	std::cout << "\n";
	size_t currentModel = context.currentModel;
	Transform* pCurrentTransform = &context.transforms[currentModel];
	pCurrentTransform->rotation =
		glm::rotate(pCurrentTransform->rotation, glm::radians(angle), glm::vec3(0.f, 0.f, 1.f));
	context.engine->setUbo({
		pCurrentTransform->translation * pCurrentTransform->rotation * pCurrentTransform->scale,
		pCurrentTransform->color
	});
}

void onNextFigureButtonPressed(EventContext& context)
{
	size_t idx = (context.engine->selectedModelIdx() + 1) % context.modelCount;
	context.currentModel = idx;
	size_t currentModel = context.currentModel;
	Transform* pCurrentTransform = &context.transforms[currentModel];
	context.engine->setUbo({
		pCurrentTransform->translation * pCurrentTransform->rotation * pCurrentTransform->scale,
		pCurrentTransform->color
	});
	context.engine->setSelectedModel(idx);
}

void updateColor(EventContext& context)
{
	size_t currentModel = context.currentModel;
	Transform* pCurrentTransform = &context.transforms[currentModel];
	glm::vec4 color = pCurrentTransform->color;
	static glm::vec4 lerpColor = RED;

	if (magnitudeSqr(color - RED) < 0.01f)
	{
		lerpColor = GRN;
	}
	else if (magnitudeSqr(color - GRN) < 0.01f)
	{
		lerpColor = BLU;
	}
	else if (magnitudeSqr(color - BLU) < 0.01f)
	{
		lerpColor = RED;
	}

	pCurrentTransform->color = lerp(pCurrentTransform->color, lerpColor, 0.0005f);
	context.engine->setUbo({
		pCurrentTransform->translation * pCurrentTransform->rotation * pCurrentTransform->scale,
		pCurrentTransform->color
	});
}*/
//
//void initUbo(const std::vector<Model2D>& models, spite::GraphicsEngine* engine)
//{
//	std::vector<const std::vector<glm::vec2>*> vertices(models.size());
//	std::vector<const std::vector<uint16_t>*> indices(models.size());
//
//	for (size_t i = 0; i < models.size(); ++i)
//	{
//		vertices[i] = &models[i].vertices;
//		indices[i] = &models[i].indices;
//	}
//
//	engine->setModelData(vertices, indices);
//	engine->setUbo({glm::mat4(1.f), {1.0f, 0.0f, 0.0f, 1.0f}});
//}

int main(int argc, char* argv[])
{
	/*
	std::vector<glm::vec2> vertices;
	std::vector<u16> indices;

	readModelInfoFile("test.txt", vertices, indices);
	Model2D testModel1(vertices, indices);

	readModelInfoFile("test2.txt", vertices, indices);
	Model2D testModel2(vertices, indices);

	readModelInfoFile("test3.txt", vertices, indices);
	Model2D testModel3(vertices, indices);
	const int modelCount = 3;

	spite::InputManager inputManager;
	spite::EventManager eventManager;
	spite::WindowManager windowManager(&eventManager, &inputManager);


	spite::GraphicsEngine engine(&windowManager);
	initUbo({testModel1, testModel2, testModel3}, &engine);

	Transform transforms[modelCount];

	EventContext context{&engine, &windowManager, modelCount, 0, transforms};
	auto startTime = std::chrono::high_resolution_clock::now();

	engine.setSelectedModel(0);
	eventManager.subscribeToEvent(spite::Events::ROTATION_BUTTON_PRESS,
	                              std::bind(onRotationButtonPressed, context));
	eventManager.subscribeToEvent(spite::Events::SCALING_BUTTON_PRESS, std::bind(onScaleButtonPressed, context));
	eventManager.subscribeToEvent(spite::Events::TRANSLATION_BUTTON_PRESS,
	                              std::bind(onTranslationButtonPressed, context));
	eventManager.subscribeToEvent(spite::Events::NEXT_FIGURE_BUTTON_PRESS,
	                              std::bind(onNextFigureButtonPressed, context));

	while (!windowManager.shouldTerminate())
	{
		windowManager.pollEvents();
		if (windowManager.shouldTerminate())
		{
			break;
		}
		eventManager.processEvents();
		eventManager.discardPollEvents();

		updateColor(context);
		engine.drawFrame();
	}
*/


	spite::HeapAllocator graphicsAllocator("Graphics allocator");
	{
		auto inputManager = std::make_shared<spite::InputManager>();
		auto eventManager = std::make_shared<spite::EventManager>();

		auto allocationCallbacks = std::make_shared<spite::AllocationCallbacksWrapper>(graphicsAllocator);

		auto windowManager = std::make_shared<spite::WindowManager>(eventManager, inputManager);

		u32 extensionCount;
		auto extensions = windowManager->getExtensions(extensionCount);

		auto core = std::make_shared<spite::CoreModule>(allocationCallbacks, extensions, extensionCount,
		                                                graphicsAllocator);
		auto base = std::make_shared<spite::BaseModule>(allocationCallbacks, core, graphicsAllocator,
		                                                windowManager->createWindowSurface(
			                                                core->instanceWrapper.instance));

		auto swapchainModule = std::make_shared<spite::SwapchainModule>(allocationCallbacks, core, base,
		                                                                graphicsAllocator, spite::WIDTH, spite::HEIGHT);

		eastl::vector<spite::Vertex, spite::HeapAllocator> vertices(graphicsAllocator);
		eastl::vector<u32, spite::HeapAllocator> indices(graphicsAllocator);

		spite::readModelInfoFile("cube.txt", vertices, indices, graphicsAllocator);
		auto model2 = std::make_shared<spite::ModelDataModule>(allocationCallbacks, base, vertices, indices);
		eastl::vector<std::shared_ptr<spite::ModelDataModule>, spite::HeapAllocator> models(graphicsAllocator);
		models.push_back(model2);
		models.push_back(model2);
		models.push_back(model2);

		auto transformUniformBuffer = std::make_shared<spite::UboModule>(
			core, base, sizeof(spite::TransformMatrix), models.size());


		auto descriptorModule = std::make_shared<spite::DescriptorModule>(allocationCallbacks,
		                                                                  base,
		                                                                  vk::DescriptorType::eUniformBufferDynamic,
		                                                                  spite::MAX_FRAMES_IN_FLIGHT,
		                                                                  0,
		                                                                  vk::ShaderStageFlagBits::eVertex,
		                                                                  transformUniformBuffer->uboBuffer,
		                                                                  transformUniformBuffer->elementAlignment,
		                                                                  graphicsAllocator);

		auto commandBuffersModule = std::make_shared<spite::GraphicsCommandModule>(allocationCallbacks,
			base, vk::CommandPoolCreateFlagBits::eResetCommandBuffer, spite::MAX_FRAMES_IN_FLIGHT);


		auto shaderService = std::make_shared<spite::ShaderServiceModule>(allocationCallbacks,
		                                                                  base, graphicsAllocator);

		spite::ShaderModuleWrapper& vertShader = shaderService->getShaderModule(
			"shaders/vert.spv", vk::ShaderStageFlagBits::eVertex);
		spite::ShaderModuleWrapper& fragShader = shaderService->getShaderModule(
			"shaders/frag.spv", vk::ShaderStageFlagBits::eFragment);

		eastl::vector<eastl::tuple<spite::ShaderModuleWrapper&, const char*>, spite::HeapAllocator> shaderModules(
			graphicsAllocator);
		shaderModules.reserve(2);
		shaderModules.push_back(eastl::make_tuple(std::ref(vertShader), "main"));
		shaderModules.push_back(eastl::make_tuple(std::ref(fragShader), "main"));


		eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator> attributes(graphicsAllocator);
		attributes = {
			vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat},
			vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat}
		};


		spite::VertexInputDescriptionsWrapper vertexInputDescriptions(
			{vk::VertexInputBindingDescription(0, sizeof(spite::Vertex), vk::VertexInputRate::eVertex)},
			attributes);


		auto cameraBindCommandPool = spite::CommandPoolWrapper(base->deviceWrapper,
		                                                       base->indices.graphicsFamily.value(),
		                                                       vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		                                                       *allocationCallbacks);

		auto cameraBindCommandBuffers = std::make_shared<spite::CommandBuffersWrapper>(base->deviceWrapper,
			cameraBindCommandPool,
			vk::CommandBufferLevel::eSecondary, 1);

		eastl::vector<std::shared_ptr<spite::CommandBuffersWrapper>, spite::HeapAllocator> extraCommandBuffers(
			graphicsAllocator);
		//extraCommandBuffers.push_back(cameraBindCommandBuffers);

		auto cameraUniformBuffer = std::make_shared<spite::UboModule>(
			core, base, sizeof(spite::CameraMatrices), 1);

		auto cameraDesciptorModule = std::make_shared<spite::DescriptorModule>(allocationCallbacks,
		                                                                       base,
		                                                                       vk::DescriptorType::eUniformBufferDynamic,
		                                                                       spite::MAX_FRAMES_IN_FLIGHT, 1,
		                                                                       vk::ShaderStageFlagBits::eVertex,
		                                                                       cameraUniformBuffer->uboBuffer,
		                                                                       cameraUniformBuffer->elementAlignment,
		                                                                       graphicsAllocator);


		eastl::vector<spite::FragmentData, spite::HeapAllocator> fragmentData(graphicsAllocator);
		fragmentData.resize(models.size());

		auto fragUbo = std::make_shared<spite::UboModule>(
			core, base, sizeof(spite::FragmentData), models.size());
		auto fragDescriptor = std::make_shared<spite::DescriptorModule>(allocationCallbacks, base,
		                                                                vk::DescriptorType::eUniformBufferDynamic,
		                                                                spite::MAX_FRAMES_IN_FLIGHT, 0,
		                                                                vk::ShaderStageFlagBits::eFragment,
		                                                                fragUbo->uboBuffer, fragUbo->elementAlignment,
		                                                                graphicsAllocator);

		eastl::vector<std::shared_ptr<spite::DescriptorModule>, spite::HeapAllocator> descriptorModules(
			graphicsAllocator);
		descriptorModules.push_back(descriptorModule);
		descriptorModules.push_back(cameraDesciptorModule);
		descriptorModules.push_back(fragDescriptor);

		auto renderModule = std::make_shared<spite::RenderModule>(allocationCallbacks, base, swapchainModule,
		                                                          descriptorModules,
		                                                          commandBuffersModule, models,
		                                                          graphicsAllocator,
		                                                          shaderModules, vertexInputDescriptions,
		                                                          windowManager,
		                                                          spite::MAX_FRAMES_IN_FLIGHT, extraCommandBuffers);


		vk::CommandBufferInheritanceInfo inheritanceInfo(swapchainModule->renderPassWrapper.renderPass, 0,
		                                                 swapchainModule->framebuffersWrapper.framebuffers[0]);


		eastl::vector<spite::Transform, spite::HeapAllocator> transforms(graphicsAllocator);
		transforms.resize(models.size() + 1);
		eastl::vector<spite::TransformMatrix, spite::HeapAllocator> transformMatrices(graphicsAllocator);
		transformMatrices.resize(models.size() + 1);

		transforms[0].position = {0.5f, 0.5f, 0.0f};
		transforms[2].position = {1.f, 1.f, 1.f};



		f32 aspectRatio = static_cast<float>(spite::WIDTH) / static_cast<float>(spite::HEIGHT);
		spite::CameraData cameraData{45.0f, aspectRatio, 0.1f, 100.0f};
		spite::CameraMatrices cameraMatrices{};

		u32 selectedModelIdx = 3;
		transforms[selectedModelIdx].position = {0.0f, 0.0f, 0.0f};
		eventManager->subscribeToEvent(spite::Events::BCKWD_BUTTON_RESS, [&transforms,&selectedModelIdx]()
		{
			glm::vec3 fwd = transforms[selectedModelIdx].rotation * glm::vec3(0.0f, 0.0f, 1.0f);
			transforms[selectedModelIdx].position += fwd * 0.1f;
		});
		eventManager->subscribeToEvent(spite::Events::FWD_BUTTON_PRESS, [&transforms,&selectedModelIdx]()
		{
			glm::vec3 fwd = transforms[selectedModelIdx].rotation * glm::vec3(0.0f, 0.0f, 1.0f);
			transforms[selectedModelIdx].position -= fwd * 0.1f;
		});
		eventManager->subscribeToEvent(spite::Events::RGHT_BUTTON_PRESS, [&transforms,&selectedModelIdx]()
		{
			glm::vec3 fwd = transforms[selectedModelIdx].rotation * glm::vec3(1.0f, 0.0f, 0.0f);
			transforms[selectedModelIdx].position += fwd * 0.1f;
		});
		eventManager->subscribeToEvent(spite::Events::LFT_BUTTON_PRESS, [&transforms,&selectedModelIdx]()
		{
			glm::vec3 fwd = transforms[selectedModelIdx].rotation * glm::vec3(1.0f, 0.0f, 0.0f);
			transforms[selectedModelIdx].position -= fwd * 0.1f;
		});

		eventManager->subscribeToEvent(spite::Events::LOOKLFT_BUTTON_PRESS, [&transforms,&selectedModelIdx]()
		{
			transforms[selectedModelIdx].rotation *= glm::angleAxis(glm::radians(5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		});
		eventManager->subscribeToEvent(spite::Events::LOOKRGHT_BUTTON_PRESS, [&transforms,&selectedModelIdx]()
		{
			transforms[selectedModelIdx].rotation *= glm::angleAxis(glm::radians(-5.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		});

		eventManager->subscribeToEvent(spite::Events::LOOKUP_BUTTON_PRESS, [&transforms,&selectedModelIdx]()
		{
			transforms[selectedModelIdx].rotation *= glm::angleAxis(glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		});
		eventManager->subscribeToEvent(spite::Events::LOOKDWN_BUTTON_PRESS, [&transforms,&selectedModelIdx]()
		{
			transforms[selectedModelIdx].rotation *= glm::angleAxis(glm::radians(-5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		});
		eventManager->subscribeToEvent(spite::Events::NEXT_FIGURE_BUTTON_PRESS, [&selectedModelIdx, &models]()
		{
			selectedModelIdx = (selectedModelIdx + 1) % (models.size() + 1);
			SDEBUG_LOG("NEXT FIG %u\n", selectedModelIdx);
		});


		/*
		auto lightCommandBuffersModule = std::make_shared<spite::GraphicsCommandModule>(allocationCallbacks,
			base, vk::CommandPoolCreateFlagBits::eResetCommandBuffer, spite::MAX_FRAMES_IN_FLIGHT);
		spite::ShaderModuleWrapper& lighFragShader = shaderService->getShaderModule(
			"shaders/lightFrag.spv", vk::ShaderStageFlagBits::eFragment);
		eastl::vector<eastl::tuple<spite::ShaderModuleWrapper&, const char*>, spite::HeapAllocator> lightShaderModules(
			graphicsAllocator);
		lightShaderModules.push_back(eastl::make_tuple(std::ref(vertShader), "main"));
		lightShaderModules.push_back(eastl::make_tuple(std::ref(lighFragShader), "main"));

		spite::readModelInfoFile("cube.txt", vertices, indices);
		auto lightSourceModel = std::make_shared<spite::ModelDataModule>(allocationCallbacks, base, vertices, indices);
		eastl::vector<std::shared_ptr<spite::ModelDataModule>, spite::HeapAllocator> lightModels(graphicsAllocator);
		models.push_back(lightSourceModel);

		auto lightRenderModule = std::make_shared<spite::RenderModule>(allocationCallbacks, base, swapchainModule,
		                                                               descriptorModules, lightCommandBuffersModule,
		                                                               lightModels,
		                                                               graphicsAllocator, lightShaderModules,
		                                                               vertexInputDescriptions, windowManager,
		                                                               spite::MAX_FRAMES_IN_FLIGHT,
		                                                               extraCommandBuffers);

		transformMatrices.push_back();
		transforms.push_back();
		*/

		fragmentData[1] = { glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) };
		fragmentData[2] = { glm::vec4(0.0f,0.0f,1.0f,1.0f) };
		u32 cameraIdx = 3;
		while (!windowManager->shouldTerminate())
		{
			renderModule->waitForFrame();
			//lightRenderModule->waitForFrame();
			windowManager->pollEvents();
			eventManager->processEvents();
			eventManager->discardPollEvents();

			transforms[0].rotation *= glm::angleAxis(glm::radians(0.01f), glm::vec3(0.0f, 1.0f, 0.0f));

			transforms[2].scale -= 0.00001f;
			spite::updateTransformMatricesSystem(transforms, transformMatrices);
			//transformMatrices[2].matrix = glm::inverse(glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
			//                                                       glm::vec3(0.0f, 0.0f, 0.0f),
			//                                                       glm::vec3(0.0f, 0.0f, 1.0f)));
			spite::updateTransformUboSystem(transformUniformBuffer->memory, transformUniformBuffer->elementAlignment,
			                                transformMatrices);

			spite::updateCameraSystem(cameraData, transformMatrices, cameraIdx, cameraMatrices);
			spite::updateCameraUboSystem(cameraUniformBuffer->memory, cameraMatrices);

			spite::updateFragUboSystem(fragUbo->memory, fragUbo->elementAlignment, fragmentData);

			renderModule->drawFrame();
			//	lightRenderModule->drawFrame();
		}
	}
	graphicsAllocator.shutdown();
}
