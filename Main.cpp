#include <chrono>
#include <iostream>

#include "GraphicsEngine.h"
#include "GraphicsUtility.h"
#include "Model2D.h"
#include "EventManager.h"
#include "WindowManager.h"
#include "AffineTransform.h"


struct Transform
{
	glm::mat4 translation = affine::identity();
	glm::mat4 rotation = affine::identity();
	glm::mat4 scale = affine::identity();
	glm::vec4 color = glm::vec4(1.f, 0.f, 0.f, 1.f);
};

glm::vec4 RED(1.f, 0.f, 0.f, 1.f);
glm::vec4 BLU(0.f, 1.f, 0.f, 1.f);
glm::vec4 GRN(0.f, 0.f, 1.f, 1.f);

struct EventContext
{
	graphics::GraphicsEngine* engine;
	application::WindowManager* windowManager;
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
	pCurrentTransform->translation *= affine::translation(x, y);

	context.engine->setUbo({
		pCurrentTransform->translation * pCurrentTransform->scale * pCurrentTransform->rotation,
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
	pCurrentTransform->scale *= affine::scale(s);
	context.engine->setUbo({
		pCurrentTransform->translation * pCurrentTransform->scale * pCurrentTransform->rotation,
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
	pCurrentTransform->rotation *= affine::rotationZ(glm::radians(angle));
	context.engine->setUbo({
		pCurrentTransform->translation * pCurrentTransform->scale * pCurrentTransform->rotation,
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
		pCurrentTransform->translation * pCurrentTransform->scale * pCurrentTransform->rotation,
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
		pCurrentTransform->translation * pCurrentTransform->scale * pCurrentTransform->rotation,
		pCurrentTransform->color
	});
}

void initUbo(const std::vector<Model2D>& models, graphics::GraphicsEngine* engine)
{
	std::vector<const std::vector<glm::vec2>*> vertices(models.size());
	std::vector<const std::vector<uint16_t>*> indices(models.size());

	for (size_t i = 0; i < models.size(); ++i)
	{
		vertices[i] = &models[i].vertices;
		indices[i] = &models[i].indices;
	}

	engine->setModelData(vertices, indices);
	engine->setUbo({affine::identity(), {1.0f, 0.0f, 0.0f, 1.0f}});
}

int main()
{
	std::vector<glm::vec2> vertices;
	std::vector<uint16_t> indices;

	readModelInfoFile("test.txt", vertices, indices);
	Model2D testModel1(vertices, indices);

	readModelInfoFile("test2.txt", vertices, indices);
	Model2D testModel2(vertices, indices);


	readModelInfoFile("test3.txt", vertices, indices);
	Model2D testModel3(vertices, indices);
	const int modelCount = 3;

	application::EventManager eventManager;
	application::WindowManager windowManager(&eventManager);

	eventManager.subscribeToEvent(application::Events::ROTATION_BUTTON_PRESS, onRotationButtonPressed);
	eventManager.subscribeToEvent(application::Events::SCALING_BUTTON_PRESS, onScaleButtonPressed);
	eventManager.subscribeToEvent(application::Events::TRANSLATION_BUTTON_PRESS, onTranslationButtonPressed);
	eventManager.subscribeToEvent(application::Events::NEXT_FIGURE_BUTTON_PRESS, onNextFigureButtonPressed);

	graphics::GraphicsEngine engine(&windowManager);
	initUbo({ testModel1, testModel2,testModel3 }, &engine);

	Transform transforms[modelCount];

	EventContext context{&engine, &windowManager, modelCount, 0, transforms};
	auto startTime = std::chrono::high_resolution_clock::now();

	engine.setSelectedModel(0);


	while (!windowManager.shouldTerminate())
	{
		updateColor(context);
		engine.drawFrame();
		windowManager.pollEvents();
		eventManager.processEvents(context);
		eventManager.discardPollEvents();
	}
}
