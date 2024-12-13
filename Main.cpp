#include <chrono>
#include <iostream>

#include <thread>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Model2D.h"

#include "Application/EventManager.hpp"
#include "Application/InputManager.hpp"
#include "Application/WindowManager.hpp"
#include "Base/Memory.hpp"
#include "Engine/Modules.hpp"
#include "Application/AppConifg.hpp"

#include "Base/File.hpp"


struct Transform
{
	glm::mat4 model;
	glm::vec4 color;

	Transform() : model(1.0f), color(1.0f)
	{
	}
};

glm::vec4 RED(1.f, 0.f, 0.f, 1.f);
glm::vec4 BLU(0.f, 1.f, 0.f, 1.f);
glm::vec4 GRN(0.f, 0.f, 1.f, 1.f);

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
		spite::InputManager inputManager;
		spite::EventManager eventManager;
		spite::WindowManager windowManager(&eventManager, &inputManager);
		u32 extensionCount;
		auto extensions = windowManager.getExtensions(extensionCount);
		auto base = std::make_shared<spite::BaseModule>(graphicsAllocator, extensions, extensionCount,
		                                                &windowManager);
		auto swapchainModule = std::make_shared<spite::SwapchainModule>(base, graphicsAllocator, 800, 600);

		auto uniformBuffer = std::make_shared<spite::UboModule>(base, sizeof(Transform), 1);


		auto descriptorModule = std::make_shared<spite::DescriptorModule>(
			base, vk::DescriptorType::eUniformBufferDynamic,
			spite::MAX_FRAMES_IN_FLIGHT,
			uniformBuffer->uboBuffer, sizeof(Transform),
			graphicsAllocator);

		auto commandBuffersModule = std::make_shared<spite::GraphicsCommandModule>(
			base, vk::CommandPoolCreateFlagBits::eResetCommandBuffer, spite::MAX_FRAMES_IN_FLIGHT);

		std::vector<glm::vec2> initVertices;
		std::vector<u32> initIndices;

		spite::readModelInfoFile("test.txt", initVertices, initIndices);

		eastl::vector<glm::vec3> vertices(initVertices.size());
		for (sizet i = 0, size = initVertices.size(); i < size; ++i)
		{
			vertices[i] = glm::vec3(initVertices[i], 0.f);
		}

		eastl::vector<u32> indices(initIndices.size());
		for (sizet i = 0, size = indices.size(); i < size; ++i)
		{
			indices[i] = initIndices[i];
		}

		auto model = std::make_shared<spite::ModelDataModule>(base, vertices, indices);
		auto shaderService = std::make_shared<spite::ShaderServiceModule>(base, graphicsAllocator);

		spite::ShaderModuleWrapper& vertShader = shaderService->getShaderModule(
			"shaders/vert.spv", vk::ShaderStageFlagBits::eVertex);
		spite::ShaderModuleWrapper& fragShader = shaderService->getShaderModule(
			"shaders/frag.spv", vk::ShaderStageFlagBits::eFragment);

		eastl::vector<eastl::tuple<spite::ShaderModuleWrapper&, const char*>, spite::HeapAllocator> shaderModules(
			graphicsAllocator);
		shaderModules.reserve(2);
		shaderModules.push_back(eastl::make_tuple(std::ref(vertShader), "main"));
		shaderModules.push_back(eastl::make_tuple(std::ref(fragShader), "main"));

		spite::VertexInputDescriptionsWrapper vertexInputDescriptions(
			{vk::VertexInputBindingDescription(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex)},
			{vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat)});
		auto renderModule = std::make_shared<spite::RenderModule>(base, swapchainModule, descriptorModule,
		                                                          commandBuffersModule, eastl::vector{model},
		                                                          graphicsAllocator,
		                                                          shaderModules, vertexInputDescriptions,
		                                                          &windowManager,
		                                                          spite::MAX_FRAMES_IN_FLIGHT);
		void* mem = uniformBuffer->uboBuffer.mapMemory();
		Transform transform;
		memcpy(mem, &transform, sizeof(transform));
		while (!windowManager.shouldTerminate())
		{
			windowManager.pollEvents();
			eventManager.processEvents();
			eventManager.discardPollEvents();
			renderModule->waitForFrame();
			renderModule->drawFrame();
		}
		uniformBuffer->uboBuffer.unmapMemory();
		base->deviceWrapper.device.waitIdle();

		windowManager.cleanup(base->instanceWrapper.instance);
	}
	graphicsAllocator.shutdown();
}
