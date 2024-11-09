#include <chrono>
#include <iostream>

#include "Model2D.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Memory.hpp"
#include "Application/EventManager.h"
#include "Engine/GraphicsEngine.h"
#include "Engine/GraphicsUtility.h"
#include "Application/WindowManager.h"

#include <EASTL/array.h>
#include <EASTL/vector.h>

struct Transform
{
	glm::mat4 translation;
	glm::mat4 rotation;
	glm::mat4 scale;
	glm::vec4 color;

	Transform() : translation(1.0f), rotation(1.0f), scale(1.0f), color(1.0f)
	{
	}
};

glm::vec4 RED(1.f, 0.f, 0.f, 1.f);
glm::vec4 BLU(0.f, 1.f, 0.f, 1.f);
glm::vec4 GRN(0.f, 0.f, 1.f, 1.f);

struct EventContext
{
	spite::GraphicsEngine* engine;
	spite::WindowManager* windowManager;
	size_t modelCount;
	size_t currentModel;
	Transform* transforms;
};

void onFrameBufferResized(EventContext& context)
{
	context.engine->framebufferResized = true;
}

void onTranslationButtonPressed(EventContext& context)
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
}

void initUbo(const eastl::vector<Model2D,spite::HeapAllocator>& models, spite::GraphicsEngine* engine)
{
	std::vector<const std::vector<glm::vec2>*> vertices(models.size());
	std::vector<const std::vector<uint16_t>*> indices(models.size());

	for (size_t i = 0; i < models.size(); ++i)
	{
		//vertices[i] = &models[i].vertices;
		//indices[i] = &models[i].indices;
	}

	engine->setModelData(vertices, indices);
	engine->setUbo({glm::mat4(1.f), {1.0f, 0.0f, 0.0f, 1.0f}});
}

int main()
{
	/*spite::HeapAllocator testAllocator("my allocator");
	{
		eastl::vector<glm::vec2, spite::HeapAllocator> vertices(testAllocator);
		eastl::vector<u16, spite::HeapAllocator> indices(testAllocator);
		vertices.set_capacity(15);
		vertices.clear();
	}
	testAllocator.shutdown();*/

	//spite::BlockAllocator blockAllocator("My block allocator");
	//int* testArray = new int;
	//blockAllocator.init(testArray, sizeof(int) * 20, sizeof(int), alignof(int));

	//for (int i = 0;i < 20; ++i)
	//{
	//	testArray[i] = i;
	//	std::cout << testArray[i];
	//}

	//readModelInfoFile("test.txt", vertices, indices);
	//Model2D testModel1(vertices, indices);

	//readModelInfoFile("test2.txt", vertices, indices);
	//Model2D testModel2(vertices, indices);

	//readModelInfoFile("test3.txt", vertices, indices);
	//Model2D testModel3(vertices, indices);
	//const int modelCount = 3;

	//spite::EventManager eventManager;
	//spite::WindowManager windowManager(&eventManager);


	//spite::GraphicsEngine engine(&windowManager);
	//initUbo({testModel1, testModel2, testModel3}, &engine);

	//Transform transforms[modelCount];

	//EventContext context{&engine, &windowManager, modelCount, 0, transforms};
	//auto startTime = std::chrono::high_resolution_clock::now();

	//engine.setSelectedModel(0);
	//eventManager.subscribeToEvent(spite::Events::ROTATION_BUTTON_PRESS,
	//                              std::bind(onRotationButtonPressed, context));
	//eventManager.subscribeToEvent(spite::Events::SCALING_BUTTON_PRESS, std::bind(onScaleButtonPressed, context));
	//eventManager.subscribeToEvent(spite::Events::TRANSLATION_BUTTON_PRESS,
	//                              std::bind(onTranslationButtonPressed, context));
	//eventManager.subscribeToEvent(spite::Events::NEXT_FIGURE_BUTTON_PRESS,
	//                              std::bind(onNextFigureButtonPressed, context));

	//while (!windowManager.shouldTerminate())
	//{
	//	updateColor(context);
	//	engine.drawFrame();
	//	windowManager.pollEvents();
	//	eventManager.processEvents();
	//	eventManager.discardPollEvents();
	//}
}
