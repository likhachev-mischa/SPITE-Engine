#include "CoreSystems.hpp"

#include "engine/VulkanGeometry.hpp"
#include "engine/VulkanResources.hpp"

namespace spite
{
	void PipelineCreateSystem::onInitialize()
	{
		auto buildInfo = m_entityService->queryBuilder()->getQueryBuildInfo();
		m_pipelineLayoutQuery = m_entityService->queryBuilder()->buildQuery<
			PipelineLayoutComponent>(buildInfo);

		buildInfo = m_entityService->queryBuilder()->getQueryBuildInfo();
		m_pipelineQuery = m_entityService->queryBuilder()->buildQuery<PipelineComponent>(buildInfo);

		requireComponent(m_pipelineCreateRequestType);
	}

	void PipelineCreateSystem::onUpdate(float deltaTime)
	{
		auto& requests = m_entityService->componentStorage()->getEventsAsserted<
			PipelineCreateRequest>();

		for (auto request : requests)
		{
			auto& shaderReference = m_entityService->componentManager()->getComponent<
				ShaderReference>(request.referencedEntity);

			auto& vertexInputComponent = m_entityService->componentManager()->getComponent<
				VertexInputComponent>(request.referencedEntity);

			Entity pipelineEntity;
			Entity layoutEntity = findCompatiblePipelineLayout(shaderReference);

			if (layoutEntity == Entity::undefined())
			{
				layoutEntity = createPipelineLayoutEntity(shaderReference);
				pipelineEntity = createPipelineEntity(layoutEntity,
				                                      vertexInputComponent,
				                                      shaderReference);
			}
			else
			{
				SDEBUG_LOG("PIPELINE LAYOUT FOUND\n")
				pipelineEntity = findCompatiblePipeline(layoutEntity,
				                                        shaderReference,
				                                        vertexInputComponent);
				if (pipelineEntity == Entity::undefined())
				{
					pipelineEntity = createPipelineEntity(layoutEntity,
					                                      vertexInputComponent,
					                                      shaderReference);
				}
			}

			// Add reference to pipeline
			PipelineReference reference;
			reference.pipelineEntity = pipelineEntity;
			m_entityService->componentManager()->addComponent<PipelineReference>(
				request.referencedEntity,
				reference);
			SDEBUG_LOG("PIPELINE REFERENCE ASSIGNED\n")
		}

		m_entityService->entityEventManager()->rewindEvent(m_pipelineCreateRequestType);
	}

	Entity PipelineCreateSystem::findCompatiblePipelineLayout(const ShaderReference& shaderRef)
	{
		auto& layoutQuery = *m_pipelineLayoutQuery;

		for (sizet i = 0, size = layoutQuery.size(); i < size; ++i)
		{
			auto& layout = layoutQuery[i];
			auto& layoutDescriptors = layout.descriptorSetLayoutEntities;
			auto& shaders = shaderRef.shaders;

			if (layoutDescriptors.size() <= shaders.size())
			{
				bool isLayoutFound = true;
				for (const Entity shader : shaders)
				{
					if (m_entityService->componentManager()->hasComponent<
						DescriptorSetLayoutComponent>(shader) && eastl::find(
						layoutDescriptors.begin(),
						layoutDescriptors.end(),
						shader) == layoutDescriptors.end())
					{
						isLayoutFound = false;
						break;
					}
				}
				if (isLayoutFound)
				{
					return layoutQuery.owner(i);
				}
			}
		}
		return Entity::undefined();
	}

	Entity PipelineCreateSystem::createPipelineLayoutEntity(const ShaderReference& shaderRef)
	{
		auto device = m_entityService->componentManager()->getSingleton<DeviceComponent>().device;
		auto allocationCallbacks = &m_entityService->componentManager()->getSingleton<
			AllocationCallbacksComponent>().allocationCallbacks;

		std::vector<vk::DescriptorSetLayout> layouts;
		std::vector<Entity> layoutEntities;
		auto& shaders = shaderRef.shaders;
		//layouts.reserve(shaders.size());
		//layoutEntities.reserve(shaders.size());

		auto componentManager = m_entityService->componentManager();
		for (const Entity shader : shaders)
		{
			if (componentManager->hasComponent<DescriptorSetLayoutComponent>(shader))
			{
				layouts.push_back(
					componentManager->getComponent<DescriptorSetLayoutComponent>(shader).layout);
				layoutEntities.push_back(shader);
			}
		}

		vk::PipelineLayout pipelineLayout = createPipelineLayout(
			device,
			layouts,
			sizeof(glm::mat4),
			allocationCallbacks);
		PipelineLayoutComponent layoutComponent;
		layoutComponent.descriptorSetLayoutEntities = std::move(layoutEntities);
		layoutComponent.layout = pipelineLayout;

		Entity layoutEntity = m_entityService->entityManager()->createEntity();
		m_entityService->componentManager()->addComponent(layoutEntity, std::move(layoutComponent));

		SDEBUG_LOG("PIPELINE LAYOUT CREATED\n")
		return layoutEntity;
	}

	Entity PipelineCreateSystem::findCompatiblePipeline(const Entity layoutEntity,
	                                                    const ShaderReference& shaderReference,
	                                                    const VertexInputComponent& vertexInput)
	{
		auto& pipelineQuery = *m_pipelineQuery;

		for (sizet i = 0, size = pipelineQuery.size(); i < size; ++i)
		{
			auto& pipelineComponent = pipelineQuery[i];
			if (pipelineComponent.pipelineLayoutEntity == layoutEntity && pipelineComponent.
				vertexInputData == vertexInput.vertexInputData)
			{
				const auto& pipelineShaderReference = m_entityService->componentManager()->
					getComponent<ShaderReference>(pipelineQuery.owner(i));

				if (pipelineShaderReference.shaders.size() != shaderReference.shaders.size())
				{
					continue;
				}

				bool isCompatible = true;
				for (const auto& shaderEntity : shaderReference.shaders)
				{
					if (eastl::find(pipelineShaderReference.shaders.begin(),
					                pipelineShaderReference.shaders.end(),
					                shaderEntity) == pipelineShaderReference.shaders.end())
					{
						isCompatible = false;
						break;
					}
				}
				if (isCompatible)
				{
					SDEBUG_LOG("COMPATIBLE PIPELINE FOUND\n")
					return pipelineQuery.owner(i);
				}
			}
		}

		return Entity::undefined();
	}

	Entity PipelineCreateSystem::createPipelineEntity(const Entity layoutEntity,
	                                                  const VertexInputComponent&
	                                                  vertexInputComponent,
	                                                  const ShaderReference& shaderReference)
	{
		//create PipelineShaderStageCreateInfos
		auto componentManager = m_entityService->componentManager();
		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
		shaderStages.reserve(shaderReference.shaders.size());
		for (const auto& shaderEntity : shaderReference.shaders)
		{
			auto& shaderComponent = componentManager->getComponent<ShaderComponent>(shaderEntity);

			vk::PipelineShaderStageCreateInfo shaderStage({},
			                                              shaderComponent.stage,
			                                              shaderComponent.shaderModule,
			                                              shaderComponent.entryPoint.c_str());
			shaderStages.push_back(shaderStage);
		}

		//create PipelineVertexInputInfo
		auto& vertexData = vertexInputComponent.vertexInputData;
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
			{},
			vertexData.bindingDescriptions.size(),
			vertexData.bindingDescriptions.data(),
			vertexData.attributeDescriptions.size(),
			vertexData.attributeDescriptions.data());


		vk::Device device = componentManager->getSingleton<DeviceComponent>().device;
		vk::PipelineLayout layout = componentManager->getComponent<PipelineLayoutComponent>(
			layoutEntity).layout;
		vk::Extent2D extent = componentManager->getSingleton<SwapchainComponent>().extent;
		vk::RenderPass renderPass = componentManager->getSingleton<GeometryRenderPassComponent>().
		                                              renderPass;
		AllocationCallbacksComponent& allocationCallbacksComponent = componentManager->getSingleton<
			AllocationCallbacksComponent>();

		vk::Pipeline pipeline = createGeometryPipeline(device,
		                                               layout,
		                                               extent,
		                                               renderPass,
		                                               shaderStages,
		                                               vertexInputInfo,
		                                               &allocationCallbacksComponent.
		                                               allocationCallbacks);

		PipelineComponent pipelineComponent;
		pipelineComponent.vertexInputData = vertexInputComponent.vertexInputData;
		pipelineComponent.pipeline = pipeline;
		pipelineComponent.pipelineLayoutEntity = layoutEntity;

		Entity pipelineEntity = m_entityService->entityManager()->createEntity();
		componentManager->addComponent(pipelineEntity, std::move(pipelineComponent));
		componentManager->addComponent(pipelineEntity, shaderReference);

		SDEBUG_LOG("PIPELINE CREATED\n")
		return pipelineEntity;
	}
}
