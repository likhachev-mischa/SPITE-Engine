#include "CoreSystems.hpp"

#include "base/File.hpp"

#include "engine/VulkanResources.hpp"

namespace spite
{
	void ShaderCreateSystem::onInitialize()
	{
		requireComponent(m_modelLoadRequestType);
	}

	void ShaderCreateSystem::onUpdate(float deltaTime)
	{
		auto& requestTable = m_entityService->componentStorage()->getEventsAsserted<
			ModelLoadRequest>();

		for (auto& request : requestTable)
		{
			Entity vertShader = findExistingShader(request.vertShaderPath.c_str());
			if (vertShader == Entity::undefined())
			{
				vertShader = createShaderEntity(request.vertShaderPath.c_str(),
				                                vk::ShaderStageFlagBits::eVertex);
				SDEBUG_LOG("VERTEX SHADER CREATED\n")
			}
			ShaderReference shaderRef;
			shaderRef.shaders.push_back(vertShader);

			Entity fragShader = findExistingShader(request.fragShaderPath.c_str());
			if (fragShader == Entity::undefined())
			{
				fragShader = createShaderEntity(request.fragShaderPath.c_str(),
				                                vk::ShaderStageFlagBits::eFragment);
				SDEBUG_LOG("FRAGMENT SHADER CREATED\n")
			}
			shaderRef.shaders.push_back(fragShader);
			m_entityService->componentManager()->addComponent<ShaderReference>(
				request.entity,
				shaderRef);

			//request pipeline creation
			PipelineCreateRequest pipelineRequest;
			pipelineRequest.referencedEntity = request.entity;
			m_entityService->entityEventManager()->createEvent(pipelineRequest);

			SDEBUG_LOG("SHADERS LOADED\n");
		}
		m_entityService->entityEventManager()->rewindEvent(m_modelLoadRequestType);
	}

	Entity ShaderCreateSystem::findExistingShader(const cstring path)
	{
		return m_entityService->entityManager()->tryGetNamedEntity(path);
	}

	Entity ShaderCreateSystem::createShaderEntity(const cstring path,
	                                              const vk::ShaderStageFlagBits& stage)
	{
		// Create shader module from file
		std::vector<char> code = readBinaryFile(path);

		auto deviceComponent = m_entityService->componentManager()->getSingleton<DeviceComponent>();
		auto allocationCallbacksComponent = m_entityService->componentManager()->getSingleton<
			AllocationCallbacksComponent>();

		// Create shader module
		vk::ShaderModule shaderModule = createShaderModule(deviceComponent.device,
		                                                   code,
		                                                   &allocationCallbacksComponent.
		                                                   allocationCallbacks);

		// Create entity for shader
		Entity shaderEntity = m_entityService->entityManager()->createEntity();
		m_entityService->entityManager()->setName(shaderEntity, path);

		// Add shader component
		ShaderComponent shader;
		shader.shaderModule = shaderModule;
		shader.stage = stage;
		shader.filePath = path;
		m_entityService->componentManager()->addComponent<ShaderComponent>(shaderEntity, shader);

		//TODO: DESCRIPTOR LAYOUT IS HARDCODED FOR NOW
		if (stage == vk::ShaderStageFlagBits::eVertex)
		{
			createDescriptors(shaderEntity, vk::ShaderStageFlagBits::eVertex);
		}

		return shaderEntity;
	}

	void ShaderCreateSystem::createDescriptors(const Entity shaderEntity,
	                                           const vk::ShaderStageFlagBits stage)
	{
		auto deviceComponent = m_entityService->componentManager()->getSingleton<DeviceComponent>();
		auto allocationCallbacksComponent = m_entityService->componentManager()->getSingleton<
			AllocationCallbacksComponent>();
		auto layout = createDescriptorSetLayout(deviceComponent.device,
		                                        {{vk::DescriptorType::eUniformBuffer, 0, stage,}},
		                                        &allocationCallbacksComponent.allocationCallbacks);
		auto pool = createDescriptorPool(deviceComponent.device,
		                                 &allocationCallbacksComponent.allocationCallbacks,
		                                 vk::DescriptorType::eUniformBuffer,
		                                 MAX_FRAMES_IN_FLIGHT);
		auto sets = createDescriptorSets(deviceComponent.device,
		                                 layout,
		                                 pool,
		                                 MAX_FRAMES_IN_FLIGHT);

		DescriptorSetLayoutComponent layoutComponent;
		layoutComponent.layout = layout;
		layoutComponent.bindingIndex = 0;
		layoutComponent.stages = stage;
		layoutComponent.type = vk::DescriptorType::eUniformBuffer;

		DescriptorPoolComponent poolComponent;
		poolComponent.maxSets = MAX_FRAMES_IN_FLIGHT;
		poolComponent.pool = pool;

		DescriptorSetsComponent setsComponent;
		std::copy_n(sets.begin(), MAX_FRAMES_IN_FLIGHT, setsComponent.descriptorSets.begin());
		//setsComponent.descriptorSets = std::move(sets);

		m_entityService->componentManager()->addComponent<DescriptorSetLayoutComponent>(
			shaderEntity,
			layoutComponent);
		m_entityService->componentManager()->addComponent<DescriptorPoolComponent>(
			shaderEntity,
			poolComponent);
		m_entityService->componentManager()->addComponent<DescriptorSetsComponent>(
			shaderEntity,
			std::move(setsComponent));
	}
}
