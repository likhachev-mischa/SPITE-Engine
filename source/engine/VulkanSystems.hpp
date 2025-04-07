#pragma once

#include <algorithm>
#include "base/File.hpp"
#include "base/Common.hpp"

#include "ecs/Core.hpp"
#include "ecs/Queries.hpp"
#include "ecs/World.hpp"

#include "engine/CoreComponents.hpp"
#include "engine/VulkanComponents.hpp"
#include "engine/ResourcesCore.hpp"
#include "engine/VulkanAllocator.hpp"
#include "engine/Debug.hpp"
#include "engine/RenderingCore.hpp"

//#include <glm/gtc/quaternion.hpp>
#include "base/Math.hpp"

namespace spite
{
	class VulkanInitSystem : public SystemBase
	{
	public:
		void onInitialize() override
		{
			auto componentManager = m_entityService->componentManager();

			vk::AllocationCallbacks allocationCallbacks = vk::AllocationCallbacks(
				m_entityService->allocatorPtr(),
				&vkAllocate,
				&vkReallocate,
				&vkFree,
				nullptr,
				nullptr);

			AllocationCallbacksComponent allocationCallbacksComponent;
			allocationCallbacksComponent.allocationCallbacks = allocationCallbacks;

			componentManager->createSingleton(allocationCallbacksComponent);

			auto windowManager = componentManager->getSingleton<WindowManagerComponent>().
			                                       windowManager;

			u32 windowExtensionsCount = 0;
			char const* const* windowExtensions = windowManager->getExtensions(
				windowExtensionsCount);
			std::vector<const char*> instanceExtensions = getRequiredExtensions(
				windowExtensions,
				windowExtensionsCount);

			vk::Instance instance = createInstance(allocationCallbacks, instanceExtensions);
			VulkanInstanceComponent instanceComponent;
			//instanceComponent.enabledExtensions = std::move(instanceExtensions);
			instanceComponent.instance = instance;
			componentManager->createSingleton(instanceComponent);

			//debug messenger
			DebugMessengerComponent debugMessenger;
			debugMessenger.messenger = createDebugUtilsMessenger(instance, &allocationCallbacks);
			componentManager->createSingleton(debugMessenger);

			vk::PhysicalDevice physicalDevice = getPhysicalDevice(instance);
			PhysicalDeviceComponent physicalDeviceComponent;
			physicalDeviceComponent.device = physicalDevice;
			physicalDeviceComponent.properties = physicalDevice.getProperties();
			physicalDeviceComponent.features = physicalDevice.getFeatures();
			physicalDeviceComponent.memoryProperties = physicalDevice.getMemoryProperties();
			componentManager->createSingleton(physicalDeviceComponent);


			vk::SurfaceKHR surface = windowManager->createWindowSurface(instance);
			SurfaceComponent surfaceComponent;
			surfaceComponent.surface = surface;
			SwapchainSupportDetails swapchainSupportDetails = querySwapchainSupport(
				physicalDevice,
				surface);
			surfaceComponent.capabilities = swapchainSupportDetails.capabilities;
			surfaceComponent.presentMode = chooseSwapPresentMode(
				swapchainSupportDetails.presentModes);
			surfaceComponent.surfaceFormat = chooseSwapSurfaceFormat(
				swapchainSupportDetails.formats);
			componentManager->createSingleton(surfaceComponent);

			QueueFamilyIndices queueFamilyIndices = findQueueFamilies(surface, physicalDevice);
			vk::Device device = createDevice(queueFamilyIndices,
			                                 physicalDevice,
			                                 &allocationCallbacks);
			DeviceComponent deviceComponent;
			deviceComponent.device = device;
			deviceComponent.queueFamilyIndices = queueFamilyIndices;
			componentManager->createSingleton(deviceComponent);

			QueueComponent queueComponent;
			queueComponent.graphicsQueue = device.getQueue(
				queueFamilyIndices.graphicsFamily.value(),
				queueComponent.graphicsQueueIndex);
			queueComponent.presentQueue = device.getQueue(queueFamilyIndices.presentFamily.value(),
			                                              queueComponent.presentQueueIndex);
			queueComponent.transferQueue = device.getQueue(
				queueFamilyIndices.transferFamily.value(),
				queueComponent.transferQueueIndex);
			componentManager->createSingleton(queueComponent);

			vma::Allocator gpuAllocator = createVmAllocator(
				physicalDevice,
				device,
				instance,
				&allocationCallbacks);
			GpuAllocatorComponent gpuAllocatorComponent;
			gpuAllocatorComponent.allocator = gpuAllocator;
			componentManager->createSingleton(gpuAllocatorComponent);

			FrameDataComponent frameData;
			componentManager->createSingleton(frameData);

			//TODO: replace constants with config
			vk::Extent2D swapExtent =
				chooseSwapExtent(surfaceComponent.capabilities, WIDTH, HEIGHT);

			vk::SwapchainKHR swapchain = createSwapchain(device,
			                                             queueFamilyIndices,
			                                             surface,
			                                             surfaceComponent.capabilities,
			                                             swapExtent,
			                                             surfaceComponent.surfaceFormat,
			                                             surfaceComponent.presentMode,
			                                             &allocationCallbacks);
			SwapchainComponent swapchainComponent;
			swapchainComponent.swapchain = swapchain;
			swapchainComponent.imageFormat = surfaceComponent.surfaceFormat.format;
			swapchainComponent.extent = swapExtent;
			swapchainComponent.images = getSwapchainImages(device, swapchain);
			swapchainComponent.imageViews = createImageViews(
				device,
				swapchainComponent.images,
				swapchainComponent.imageFormat,
				&allocationCallbacks);
			componentManager->createSingleton(swapchainComponent);

			CommandPoolComponent commandPoolComponent;
			commandPoolComponent.graphicsCommandPool = createCommandPool(
				device,
				&allocationCallbacks,
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				queueFamilyIndices.graphicsFamily.value());
			commandPoolComponent.transferCommandPool = createCommandPool(
				device,
				&allocationCallbacks,
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				queueFamilyIndices.transferFamily.value());
			componentManager->createSingleton(commandPoolComponent);

			CommandBufferComponent cbComponent;
			//cbComponent.primaryBuffers = createGraphicsCommandBuffers(
			//	device,
			//	commandPoolComponent.graphicsCommandPool,
			//	vk::CommandBufferLevel::ePrimary,
			//	MAX_FRAMES_IN_FLIGHT);
			//cbComponent.secondaryBuffers = createGraphicsCommandBuffers(
			//	device,
			//	commandPoolComponent.graphicsCommandPool,
			//	vk::CommandBufferLevel::eSecondary,
			//	MAX_FRAMES_IN_FLIGHT);

			std::copy_n(
				createGraphicsCommandBuffers(device,
				                             commandPoolComponent.graphicsCommandPool,
				                             vk::CommandBufferLevel::ePrimary,
				                             MAX_FRAMES_IN_FLIGHT).begin(),
				MAX_FRAMES_IN_FLIGHT,
				cbComponent.primaryBuffers.begin());
			std::copy_n(
				createGraphicsCommandBuffers(device,
				                             commandPoolComponent.graphicsCommandPool,
				                             vk::CommandBufferLevel::eSecondary,
				                             MAX_FRAMES_IN_FLIGHT).begin(),
				MAX_FRAMES_IN_FLIGHT,
				cbComponent.secondaryBuffers.begin());

			componentManager->createSingleton(std::move(cbComponent));


			//TODO: smart pipeline creation
			RenderPassComponent renderPassComponent;
			renderPassComponent.renderPass = createRenderPass(
				device,
				swapchainComponent.imageFormat,
				&allocationCallbacks);
			componentManager->createSingleton(renderPassComponent);

			FramebufferComponent framebufferComponent;
			framebufferComponent.framebuffers = createFramebuffers(
				device,
				swapchainComponent.imageViews,
				swapExtent,
				renderPassComponent.renderPass,
				&allocationCallbacks);
			componentManager->createSingleton(std::move(framebufferComponent));

			std::vector<vk::Semaphore> imageAvailableSemaphores;
			imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
			std::vector<vk::Semaphore> renderFinishedSemaphores;
			renderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
			std::vector<vk::Fence> inFlightFences;
			inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
			for (sizet i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				imageAvailableSemaphores.push_back(
					createSemaphore(device, {}, &allocationCallbacks));
				renderFinishedSemaphores.push_back(
					createSemaphore(device, {}, &allocationCallbacks));
				inFlightFences.push_back(createFence(device,
				                                     {vk::FenceCreateFlagBits::eSignaled},
				                                     &allocationCallbacks));
			}

			SynchronizationComponent synchronizationComponent;
			//		synchronizationComponent.imageAvailableSemaphores = std::move(imageAvailableSemaphores);
			//		synchronizationComponent.renderFinishedSemaphores = std::move(renderFinishedSemaphores);
			//		synchronizationComponent.inFlightFences = std::move(inFlightFences);
			std::copy_n(imageAvailableSemaphores.begin(),
			            MAX_FRAMES_IN_FLIGHT,
			            synchronizationComponent.imageAvailableSemaphores.begin());
			std::copy_n(renderFinishedSemaphores.begin(),
			            MAX_FRAMES_IN_FLIGHT,
			            synchronizationComponent.renderFinishedSemaphores.begin());
			std::copy_n(inFlightFences.begin(),
			            MAX_FRAMES_IN_FLIGHT,
			            synchronizationComponent.inFlightFences.begin());

			componentManager->createSingleton(std::move(synchronizationComponent));

			SDEBUG_LOG("VULKAN INITIALIZED\n")
			//	requireComponent(typeid(VulkanInitRequest));
		}
	};

	class CameraCreateSystem : public SystemBase
	{
	public:
		void onInitialize() override
		{
			Entity camera = m_entityService->entityManager()->createEntity();

			auto componentManager = m_entityService->componentManager();

			componentManager->addComponent<TransformComponent>(camera);
			componentManager->addComponent<TransformMatrixComponent>(camera);
			componentManager->addComponent<CameraDataComponent>(camera);
			componentManager->addComponent<CameraMatricesComponent>(camera);

			CameraSingleton cameraSingleton;
			cameraSingleton.camera = camera;
			componentManager->createSingleton(cameraSingleton);


			auto& physicalDeviceComponent = componentManager->getSingleton<
				PhysicalDeviceComponent>();
			auto queueIndices = componentManager->getSingleton<DeviceComponent>().
			                                      queueFamilyIndices;
			auto gpuAllocator = componentManager->getSingleton<GpuAllocatorComponent>().allocator;

			UniformBufferSharedComponent cameraUbo;

			cameraUbo.descriptorType = vk::DescriptorType::eUniformBuffer;
			cameraUbo.shaderStage = vk::ShaderStageFlagBits::eVertex;
			cameraUbo.bindingIndex = 0;

			//will pass viewprojection matrix
			sizet elementSize = sizeof(glm::mat4);
			cameraUbo.elementSize = elementSize;
			cameraUbo.elementCount = 1;
			sizet minUboAlignment = physicalDeviceComponent.properties.limits.
			                                                minUniformBufferOffsetAlignment;
			sizet dynamicAlignment = (elementSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
			cameraUbo.elementAlignment = dynamicAlignment;
			for (sizet i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				cameraUbo.ubos[i].buffer = BufferWrapper(elementSize,
				                                         vk::BufferUsageFlagBits::eUniformBuffer,
				                                         vk::MemoryPropertyFlagBits::eHostVisible |
				                                         vk::MemoryPropertyFlagBits::eHostCoherent,
				                                         vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
				                                         queueIndices,
				                                         gpuAllocator);

				cameraUbo.ubos[i].mappedMemory = cameraUbo.ubos[i].buffer.mapMemory();
			}

			m_entityService->componentManager()->addComponent(camera, std::move(cameraUbo));
		}
	};

	class ModelLoadSystem : public SystemBase
	{
	public:
		void onInitialize() override
		{
			requireComponent(typeid(ModelLoadRequest));
		}

		void onUpdate(float deltaTime) override
		{
			auto& requestTable = m_entityService->componentStorage()->getEventsAsserted<
				ModelLoadRequest>();


			for (auto& request : requestTable)
			{
				Entity entity;
				if (request.entity == Entity::undefined())
				{
					entity = m_entityService->entityManager()->createEntity();
					request.entity = entity;

					if (!request.name.empty())
					{
						m_entityService->entityManager()->setName(entity, request.name.c_str());
					}
				}
				else
				{
					entity = request.entity;
				}

				// Load vertices and indices from .obj file
				eastl::vector<Vertex, HeapAllocator> vertices(m_entityService->allocator());
				eastl::vector<u32, HeapAllocator> indices(m_entityService->allocator());
				readModelInfoFile(request.objFilePath.c_str(),
				                  vertices,
				                  indices,
				                  m_entityService->allocator());

				// Create mesh component
				MeshComponent mesh;
				createModelBuffers(entity, vertices, indices, mesh);
				m_entityService->componentManager()->addComponent<MeshComponent>(
					entity,
					std::move(mesh));

				VertexInputData vertexInputData;
				assingVertexInput(vertexInputData);

				VertexInputComponent vertexInputComponent;
				vertexInputComponent.vertexInputData = std::move(vertexInputData);
				m_entityService->componentManager()->addComponent(
					entity,
					std::move(vertexInputComponent));

				if (!m_entityService->componentManager()->hasComponent(
					entity,
					typeid(TransformComponent)))
				{
					TransformComponent transform;
					m_entityService->componentManager()->addComponent<TransformComponent>(
						entity,
						transform);
				}

				if (!m_entityService->componentManager()->hasComponent(
					entity,
					typeid(TransformMatrixComponent)))
				{
					TransformMatrixComponent matrix;
					m_entityService->componentManager()->addComponent<TransformMatrixComponent>(
						entity,
						matrix);
				}

				SDEBUG_LOG("MODEL LOADED\n")
			}
		}

	private:
		void createModelBuffers(Entity entity,
		                        const eastl::vector<Vertex, HeapAllocator>& vertices,
		                        const eastl::vector<u32, HeapAllocator>& indices,
		                        MeshComponent& mesh)
		{
			mesh.indexCount = indices.size();

			auto deviceComponent = m_entityService->componentManager()->getSingleton<
				DeviceComponent>();
			auto gpuAllocatorComponent = m_entityService->componentManager()->getSingleton<
				GpuAllocatorComponent>();
			auto commandPoolComponent = m_entityService->componentManager()->getSingleton<
				CommandPoolComponent>();
			auto queueComponent = m_entityService->componentManager()->getSingleton<
				QueueComponent>();
			auto& allocationCallbacksComponent = m_entityService->componentManager()->getSingleton<
				AllocationCallbacksComponent>();

			sizet vertSize = vertices.size() * sizeof(vertices[0]);
			sizet indSize = indices.size() * sizeof(indices[0]);

			mesh.indexBuffer = BufferWrapper(indSize,
			                                 vk::BufferUsageFlagBits::eIndexBuffer |
			                                 vk::BufferUsageFlagBits::eTransferDst,
			                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
			                                 {},
			                                 deviceComponent.queueFamilyIndices,
			                                 gpuAllocatorComponent.allocator);

			auto indexStage = BufferWrapper(indSize,
			                                vk::BufferUsageFlagBits::eTransferSrc |
			                                vk::BufferUsageFlagBits::eIndexBuffer,
			                                vk::MemoryPropertyFlagBits::eHostVisible |
			                                vk::MemoryPropertyFlagBits::eHostCoherent,
			                                vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
			                                | vma::AllocationCreateFlagBits::eStrategyMinTime,
			                                deviceComponent.queueFamilyIndices,
			                                gpuAllocatorComponent.allocator);

			//indexStage.copyMemory(vertices.data(), vertSize, 0);
			indexStage.copyMemory(indices.data(), indSize, 0);

			mesh.indexBuffer.copyBuffer(indexStage,
			                            deviceComponent.device,
			                            commandPoolComponent.transferCommandPool,
			                            queueComponent.transferQueue,
			                            &allocationCallbacksComponent.allocationCallbacks);

			mesh.vertexBuffer = BufferWrapper(vertSize,
			                                  vk::BufferUsageFlagBits::eVertexBuffer |
			                                  vk::BufferUsageFlagBits::eTransferDst,
			                                  vk::MemoryPropertyFlagBits::eDeviceLocal,
			                                  {},
			                                  deviceComponent.queueFamilyIndices,
			                                  gpuAllocatorComponent.allocator);

			auto vertexStage = BufferWrapper(vertSize,
			                                 vk::BufferUsageFlagBits::eTransferSrc |
			                                 vk::BufferUsageFlagBits::eVertexBuffer,
			                                 vk::MemoryPropertyFlagBits::eHostVisible |
			                                 vk::MemoryPropertyFlagBits::eHostCoherent,
			                                 vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
			                                 | vma::AllocationCreateFlagBits::eStrategyMinTime,
			                                 deviceComponent.queueFamilyIndices,
			                                 gpuAllocatorComponent.allocator);

			vertexStage.copyMemory(vertices.data(), vertSize, 0);

			mesh.vertexBuffer.copyBuffer(vertexStage,
			                             deviceComponent.device,
			                             commandPoolComponent.transferCommandPool,
			                             queueComponent.transferQueue,
			                             &allocationCallbacksComponent.allocationCallbacks);
		}

		//hardcoded for now
		void assingVertexInput(VertexInputData& vertexInputData)
		{
			vertexInputData.bindingDescriptions.push_back({0, sizeof(Vertex)});

			vertexInputData.attributeDescriptions.reserve(2);
			vertexInputData.attributeDescriptions.push_back({0, 0, vk::Format::eR32G32B32Sfloat});
			vertexInputData.attributeDescriptions.push_back({1, 0, vk::Format::eR32G32B32Sfloat});
		}
	};

	struct PipelineCreateRequest : IEventComponent
	{
		//entity(model) that requires pipeline creation/accuisition
		//has ShaderReference component 
		Entity referencedEntity;
	};

	struct ShaderReference : IComponent
	{
		std::vector<Entity> shaders{};
	};

	//creates named entites for ShaderComponents
	//name is filepath 
	class ShaderCreateSystem : public SystemBase
	{
		std::type_index m_modelLoadRequestType = typeid(ModelLoadRequest);

	public:
		void onInitialize() override
		{
			requireComponent(m_modelLoadRequestType);
		}

		//TODO: parse .spirv to create descriptorset layout automatically
		void onUpdate(float deltaTime) override
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

	private:
		Entity findExistingShader(const cstring path)
		{
			return m_entityService->entityManager()->tryGetNamedEntity(path);
		}

		Entity createShaderEntity(const cstring path, const vk::ShaderStageFlagBits& stage)
		{
			// Create shader module from file
			std::vector<char> code = readBinaryFile(path);

			auto deviceComponent = m_entityService->componentManager()->getSingleton<
				DeviceComponent>();
			auto allocationCallbacksComponent = m_entityService->componentManager()->getSingleton<
				AllocationCallbacksComponent>();

			// Create shader module
			vk::ShaderModule shaderModule = createShaderModule(
				deviceComponent.device,
				code,
				&allocationCallbacksComponent.allocationCallbacks);

			// Create entity for shader
			Entity shaderEntity = m_entityService->entityManager()->createEntity();
			m_entityService->entityManager()->setName(shaderEntity, path);

			// Add shader component
			ShaderComponent shader;
			shader.shaderModule = shaderModule;
			shader.stage = stage;
			shader.filePath = path;
			m_entityService->componentManager()->addComponent<
				ShaderComponent>(shaderEntity, shader);

			//TODO: DESCRIPTOR LAYOUT IS HARDCODED FOR NOW
			if (stage == vk::ShaderStageFlagBits::eVertex)
			{
				createDescriptors(shaderEntity, vk::ShaderStageFlagBits::eVertex);
			}

			return shaderEntity;
		}

		void createDescriptors(const Entity shaderEntity, const vk::ShaderStageFlagBits stage)
		{
			auto deviceComponent = m_entityService->componentManager()->getSingleton<
				DeviceComponent>();
			auto allocationCallbacksComponent = m_entityService->componentManager()->getSingleton<
				AllocationCallbacksComponent>();
			auto layout = createDescriptorSetLayout(deviceComponent.device,
			                                        vk::DescriptorType::eUniformBuffer,
			                                        0,
			                                        stage,
			                                        &allocationCallbacksComponent.
			                                        allocationCallbacks);
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
	};

	struct PipelineReference : IComponent
	{
		Entity pipelineEntity;
	};

	class PipelineCreateSystem : public SystemBase
	{
		std::type_index m_pipelineCreateRequestType = typeid(PipelineCreateRequest);

		Query1<PipelineLayoutComponent>* m_pipelineLayoutQuery;
		Query1<PipelineComponent>* m_pipelineQuery;

	public:
		void onInitialize() override
		{
			auto buildInfo = m_entityService->queryBuilder()->getQueryBuildInfo();
			m_pipelineLayoutQuery = m_entityService->queryBuilder()->buildQuery<
				PipelineLayoutComponent>(buildInfo);

			buildInfo = m_entityService->queryBuilder()->getQueryBuildInfo();
			m_pipelineQuery = m_entityService->queryBuilder()->buildQuery<PipelineComponent>(
				buildInfo);

			requireComponent(m_pipelineCreateRequestType);
		}

		void onUpdate(float deltaTime) override
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
					pipelineEntity = findCompatiblePipeline(layoutEntity, vertexInputComponent);
					if (pipelineEntity == Entity::undefined())
					{
						pipelineEntity = createPipelineEntity(
							layoutEntity,
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

	private:
		//assumes that shader entity == descriptorSetLayout entity
		Entity findCompatiblePipelineLayout(const ShaderReference& shaderRef)
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
						return layoutQuery.componentOwner(i);
					}
				}
			}
			return Entity::undefined();
		}

		Entity createPipelineLayoutEntity(const ShaderReference& shaderRef)
		{
			auto device = m_entityService->componentManager()->getSingleton<DeviceComponent>().
			                               device;
			auto allocationCallbacks = &m_entityService->componentManager()->getSingleton<
				AllocationCallbacksComponent>().allocationCallbacks;

			std::vector<vk::DescriptorSetLayout> layouts;
			std::vector<Entity> layoutEntities;
			auto& shaders = shaderRef.shaders;
			layouts.reserve(shaders.size());
			layoutEntities.reserve(shaders.size());

			auto componentManager = m_entityService->componentManager();
			for (const Entity shader : shaders)
			{
				if (componentManager->hasComponent<DescriptorSetLayoutComponent>(shader))
				{
					layouts.push_back(
						componentManager->getComponent<DescriptorSetLayoutComponent>(
							shader).layout);
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
			m_entityService->componentManager()->addComponent(
				layoutEntity,
				std::move(layoutComponent));

			SDEBUG_LOG("PIPELINE LAYOUT CREATED\n")
			return layoutEntity;
		}

		Entity findCompatiblePipeline(const Entity layoutEntity,
		                              const VertexInputComponent& vertexInput)
		{
			auto& pipelineQuery = *m_pipelineQuery;

			for (sizet i = 0, size = pipelineQuery.size(); i < size; ++i)
			{
				auto& pipelineComponent = pipelineQuery[i];
				if (pipelineComponent.pipelineLayoutEntity == layoutEntity && pipelineComponent.
					vertexInputData == vertexInput.vertexInputData)
				{
					return pipelineQuery.componentOwner(i);
				}
			}

			return Entity::undefined();
		}

		Entity createPipelineEntity(const Entity layoutEntity,
		                            const VertexInputComponent& vertexInputComponent,
		                            const ShaderReference& shaderReference)
		{
			//create PipelineShaderStageCreateInfos
			auto componentManager = m_entityService->componentManager();
			std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
			shaderStages.reserve(shaderReference.shaders.size());
			for (const auto& shaderEntity : shaderReference.shaders)
			{
				auto& shaderComponent = componentManager->getComponent<ShaderComponent>(
					shaderEntity);

				vk::PipelineShaderStageCreateInfo shaderStage(
					{},
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
			vk::RenderPass renderPass = componentManager->getSingleton<RenderPassComponent>().
			                                              renderPass;
			AllocationCallbacksComponent& allocationCallbacksComponent = componentManager->
				getSingleton<AllocationCallbacksComponent>();

			vk::Pipeline pipeline = createGraphicsPipeline(device,
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

			SDEBUG_LOG("PIPELINE CREATED\n")
			return pipelineEntity;
		}
	};

	class TransformationMatrixSystem : public SystemBase
	{
		Query2<TransformComponent, TransformMatrixComponent>* m_query;

	public:
		void onInitialize() override
		{
			m_query = m_entityService->queryBuilder()->buildQuery<
				TransformComponent, TransformMatrixComponent>();
		}

		void onUpdate(float deltaTime) override
		{
			auto& query = *m_query;
			for (sizet i = 0, size = query.size(); i < size; ++i)
			{
				auto& transform = query.getComponentT1(i);
				//only recalculate dirty matrices 
				if (!transform.isDirty)
				{
					continue;
				}

				auto& matrix = query.getComponentT2(i);
				matrix.matrix = calculateModelMatrix(query.owner(i), transform);
			}
		}

	private:
		glm::mat4 calculateModelMatrix(Entity entity, TransformComponent& transform)
		{
			// Calculate local transformation
			glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), transform.position) *
				glm::mat4_cast(transform.rotation) * glm::scale(glm::mat4(1.0f), transform.scale);

			transform.isDirty = false;

			// Apply parent transformation if any
			if (transform.parent != Entity::undefined())
			{
				TransformComponent& parentTransform = m_entityService->componentManager()->
					getComponent<TransformComponent>(transform.parent);

				glm::mat4 parentMatrix;
				if (parentTransform.isDirty)
				{
					parentMatrix = calculateModelMatrix(transform.parent, parentTransform);
				}
				else
				{
					parentMatrix = m_entityService->componentManager()->getComponent<
						TransformMatrixComponent>(transform.parent).matrix;
				}
				return parentMatrix * localMatrix;
			}

			return localMatrix;
		}
	};

	class CameraMatricesUpdateSystem : public SystemBase
	{
		Query2<TransformComponent, CameraMatricesComponent>* m_query1;
		Query2<CameraDataComponent, CameraMatricesComponent>* m_query2;

	public:
		void onInitialize() override
		{
			m_query1 = m_entityService->queryBuilder()->buildQuery<
				TransformComponent, CameraMatricesComponent>();
			m_query2 = m_entityService->queryBuilder()->buildQuery<
				CameraDataComponent, CameraMatricesComponent>();
		}

		void onUpdate(float deltaTime) override
		{
			auto& query1 = *m_query1;

			for (sizet i = 0, size = query1.size(); i < size; ++i)
			{
				TransformComponent& transform = query1.getComponentT1(i);
				CameraMatricesComponent& matrices = query1.getComponentT2(i);

				//matrices.view = createViewMatrix(transform.position, transform.rotation);
				matrices.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
				                            glm::vec3(0.0f, 0.0f, 0.0f),
				                            glm::vec3(0.0f, 0.0f, 1.0f));
			}

			auto& query2 = *m_query2;

			//TODO: store aspect in swapchain

			auto& swapchainComponent = m_entityService->componentManager()->getSingleton<
				SwapchainComponent>();
			float aspect = swapchainComponent.extent.width / static_cast<float>(swapchainComponent.
				extent.height);
			for (sizet i = 0, size = query2.size(); i < size; ++i)
			{
				CameraDataComponent& data = query2.getComponentT1(i);
				//update projection matrix only when neccesary
				if (!data.isDirty)
				{
					continue;
				}
				CameraMatricesComponent& matrices = query2.getComponentT2(i);
				matrices.projection = createProjectionMatrix(data, aspect);

				//matrices.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
				//matrices.projection[1][1] *= -1;

				//data.isDirty = false;
			}
		}

	private:
		glm::mat4 createViewMatrix(const glm::vec3& position, const glm::quat& rotation)
		{
			glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);

			glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

			glm::vec3 target = position + forward;

			return glm::lookAt(position, target, up);
		}

		glm::mat4 createProjectionMatrix(CameraDataComponent& data, float aspect)
		{
			data.isDirty = false;

			glm::mat4 proj = glm::perspective(data.fov, aspect, data.nearPlane, data.farPlane);
			proj[1][1] *= -1;
			return proj;
		}
	};


	class CameraUboUpdateSystem : public SystemBase
	{
	public:
		void onUpdate(float deltaTime) override
		{
			Entity camera = m_entityService->componentManager()->getSingleton<CameraSingleton>().
			                                 camera;

			auto& cameraUbo = m_entityService->componentManager()->getComponent<
				UniformBufferSharedComponent>(camera);

			auto& cameraMatrices = m_entityService->componentManager()->getComponent<
				CameraMatricesComponent>(camera);

			u32 currentFrame = m_entityService->componentManager()->getSingleton<
				FrameDataComponent>().currentFrame;

			glm::mat4 viewProjection = cameraMatrices.projection * cameraMatrices.view;

			//SDEBUG_LOG("camera UBO Updated\n")
			memcpy(cameraUbo.ubos[currentFrame].mappedMemory,
			       &viewProjection,
			       cameraUbo.elementSize);
		}
	};

	class WaitForFrameSystem : public SystemBase
	{
	public:
		void onInitialize() override
		{
			requireComponent(typeid(PipelineComponent));
		}

		void onUpdate(float deltaTime) override
		{
			FrameDataComponent& frameData = m_entityService->componentManager()->getSingleton<
				FrameDataComponent>();

			u32 imageIndex = frameData.imageIndex;
			u32 currentFrame = frameData.currentFrame;

			vk::SwapchainKHR swapchain = m_entityService->componentManager()->getSingleton<
				SwapchainComponent>().swapchain;

			SynchronizationComponent& synchronizationComponent = m_entityService->componentManager()
				->getSingleton<SynchronizationComponent>();
			vk::Device device = m_entityService->componentManager()->getSingleton<DeviceComponent>()
			                                   .device;

			vk::Result result = waitForFrame(device,
			                                 swapchain,
			                                 synchronizationComponent.inFlightFences[currentFrame],
			                                 synchronizationComponent.imageAvailableSemaphores[
				                                 currentFrame],
			                                 imageIndex);
			SASSERT_VULKAN(result);

			frameData.imageIndex = imageIndex;
		}
	};

	//TODO peredelat
	class DescriptorUpdateSystem : public SystemBase
	{
		Query2<DescriptorSetLayoutComponent, DescriptorSetsComponent>* m_descriptorQuery;

		Query1<UniformBufferComponent>* m_uboQuery;
		SharedQuery1<UniformBufferSharedComponent>* m_uboSharedQuery;

	public:
		void onInitialize() override
		{
			m_descriptorQuery = m_entityService->queryBuilder()->buildQuery<
				DescriptorSetLayoutComponent, DescriptorSetsComponent>();
			m_uboQuery = m_entityService->queryBuilder()->buildQuery<UniformBufferComponent>();

			m_uboSharedQuery = m_entityService->queryBuilder()->buildQuery<
				UniformBufferSharedComponent>();

			requireComponent(typeid(PipelineComponent));
		}

		void onUpdate(float deltaTime) override
		{
			u32 currentFrame = m_entityService->componentManager()->getSingleton<
				FrameDataComponent>().currentFrame;

			auto device = m_entityService->componentManager()->getSingleton<DeviceComponent>().
			                               device;

			auto& descriptorQuery = *m_descriptorQuery;
			auto& uboSharedQuery = *m_uboSharedQuery;
			auto& uboQuery = *m_uboQuery;

			for (sizet i = 0, size = descriptorQuery.size(); i < size; ++i)
			{
				auto& descriptorLayoutComponent = descriptorQuery.getComponentT1(i);

				auto descriptorType = descriptorLayoutComponent.type;
				auto descriptorStage = descriptorLayoutComponent.stages;

				auto descriptor = descriptorQuery.getComponentT2(i);
				for (sizet j = 0, sizej = uboSharedQuery.size(); j < sizej; ++j)
				{
					auto& ubo = uboSharedQuery[j];

					if (ubo.descriptorType != descriptorType || ubo.shaderStage != descriptorStage)
					{
						continue;
					}

					updateDescriptorSets(device,
					                     descriptor.descriptorSets[currentFrame],
					                     ubo.ubos[currentFrame].buffer.buffer,
					                     ubo.descriptorType,
					                     ubo.bindingIndex,
					                     ubo.elementSize);
					//SDEBUG_LOG("DESCRIPTOR UPDATED\n")
				}

				for (sizet j = 0, sizej = uboQuery.size(); j < sizej; ++j)
				{
					auto& ubo = uboQuery[j];

					if (ubo.descriptorType != descriptorType || ubo.shaderStage != descriptorStage)
					{
						continue;
					}

					updateDescriptorSets(device,
					                     descriptor.descriptorSets[currentFrame],
					                     ubo.ubos[currentFrame].buffer.buffer,
					                     ubo.descriptorType,
					                     ubo.bindingIndex,
					                     ubo.elementSize);
					//SDEBUG_LOG("DESCRIPTOR UPDATED\n")
				}
			}
		}
	};


	class RenderSystem : public SystemBase
	{
	private:
		Query1<PipelineComponent>* m_pipelineQuery;
		Query2<MeshComponent, PipelineReference>* m_modelQuery;

	public:
		void onInitialize() override
		{
			m_pipelineQuery = m_entityService->queryBuilder()->buildQuery<PipelineComponent>();

			m_modelQuery = m_entityService->queryBuilder()->buildQuery<
				MeshComponent, PipelineReference>();

			SDEBUG_LOG("RENDER SYSTEM INIT\n")
			requireComponent(typeid(PipelineComponent));
		}

		//TODO: CREATE DESCRIPTOR SETS FOR PIPELINE INDIVIDUALLY
		void onUpdate(float deltaTime) override
		{
			FrameDataComponent& frameData = m_entityService->componentManager()->getSingleton<
				FrameDataComponent>();

			u32 imageIndex = frameData.imageIndex;
			u32 currentFrame = frameData.currentFrame;

			//SDEBUG_LOG("RENDER SYSTEM UPDATE START\n")
			vk::Device device = m_entityService->componentManager()->getSingleton<DeviceComponent>()
			                                   .device;

			SwapchainComponent& swapchainComponent = m_entityService->componentManager()->
				getSingleton<SwapchainComponent>();
			vk::SwapchainKHR swapchain = swapchainComponent.swapchain;
			vk::Extent2D extent = swapchainComponent.extent;

			FramebufferComponent& fbComponent = m_entityService->componentManager()->getSingleton<
				FramebufferComponent>();
			vk::RenderPass renderPass = m_entityService->componentManager()->getSingleton<
				RenderPassComponent>().renderPass;

			SynchronizationComponent& synchronizationComponent = m_entityService->componentManager()
				->getSingleton<SynchronizationComponent>();

			//vk::Result result = waitForFrame(device,
			//                                 swapchain,
			//                                 synchronizationComponent.inFlightFences[currentFrame],
			//                                 synchronizationComponent.imageAvailableSemaphores[
			//	                                 currentFrame],
			//                                 imageIndex);
			//SASSERT_VULKAN(result);

			CommandBufferComponent& cbComponent = m_entityService->componentManager()->getSingleton<
				CommandBufferComponent>();

			auto& pipelineQuery = *m_pipelineQuery;
			auto& modelQuery = *m_modelQuery;

			beginSecondaryCommandBuffer(cbComponent.secondaryBuffers[currentFrame],
			                            renderPass,
			                            fbComponent.framebuffers[imageIndex]);
			for (sizet i = 0, size = pipelineQuery.size(); i < size; ++i)
			{
				auto& pipeline = pipelineQuery[i];

				auto layoutComponent = m_entityService->componentManager()->getComponent<
					PipelineLayoutComponent>(pipeline.pipelineLayoutEntity);

				//TODO assemble descriptor sets for many shaders
				DescriptorSetsComponent& descriptorSets = m_entityService->componentManager()->
					getComponent<DescriptorSetsComponent>(
						layoutComponent.descriptorSetLayoutEntities[0]);

				Entity pipelineEntity = pipelineQuery.componentOwner(i);

				for (sizet j = 0, size2 = modelQuery.size(); j < size2; ++j)
				{
					PipelineReference& pipelineRef = modelQuery.getComponentT2(j);
					//if model is referenced to this pipeline
					if (pipelineEntity != pipelineRef.pipelineEntity)
					{
						continue;
					}

					u32 offsets = 0;
					MeshComponent& meshComponent = modelQuery.getComponentT1(j);

					auto& transformMatrixComponent = m_entityService->componentManager()->
						getComponent<TransformMatrixComponent>(modelQuery.owner(j));
					cbComponent.secondaryBuffers[currentFrame].pushConstants(
						layoutComponent.layout,
						vk::ShaderStageFlagBits::eVertex,
						0,
						sizeof(glm::mat4),
						&transformMatrixComponent.matrix);

					recordSecondaryCommandBuffer(cbComponent.secondaryBuffers[currentFrame],
					                             pipeline.pipeline,
					                             layoutComponent.layout,
					                             {descriptorSets.descriptorSets[currentFrame]},
					                             extent,
					                             meshComponent.indexBuffer.buffer,
					                             meshComponent.vertexBuffer.buffer,
					                             &offsets,
					                             0,
					                             meshComponent.indexCount);
				}
			}
			endSecondaryCommandBuffer(cbComponent.secondaryBuffers[currentFrame]);

			recordPrimaryCommandBuffer(cbComponent.primaryBuffers[currentFrame],
			                           extent,
			                           renderPass,
			                           fbComponent.framebuffers[imageIndex],
			                           {cbComponent.secondaryBuffers[currentFrame]},
			                           swapchainComponent.images[imageIndex]);

			QueueComponent& queueComponent = m_entityService->componentManager()->getSingleton<
				QueueComponent>();

			drawFrame(cbComponent.primaryBuffers[currentFrame],
			          synchronizationComponent.inFlightFences[currentFrame],
			          synchronizationComponent.imageAvailableSemaphores[currentFrame],
			          synchronizationComponent.renderFinishedSemaphores[currentFrame],
			          queueComponent.graphicsQueue,
			          queueComponent.presentQueue,
			          swapchain,
			          imageIndex);

			//SDEBUG_LOG("FRAME RENDERED\n");
			currentFrame = (currentFrame + 1) % (MAX_FRAMES_IN_FLIGHT);

			frameData.imageIndex = imageIndex;
			frameData.currentFrame = currentFrame;
		}
	};

	struct CleanupRequest : IEventComponent
	{
	};

	class CleanupSystem : public SystemBase
	{
	public:
		void onInitialize() override
		{
			requireComponent(typeid(CleanupRequest));
		}

		void onUpdate(float deltaTime) override
		{
			auto componentManager = m_entityService->componentManager();

			AllocationCallbacksComponent& allocationCallbacksComponent = componentManager->
				getSingleton<AllocationCallbacksComponent>();
			vk::AllocationCallbacks* allocationCallbacks = &allocationCallbacksComponent.
				allocationCallbacks;

			GpuAllocatorComponent& gpuAllocatorComponent = componentManager->getSingleton<
				GpuAllocatorComponent>();

			vk::Instance instance = componentManager->getSingleton<VulkanInstanceComponent>().
			                                          instance;
			//vk::PhysicalDevice physicalDevice = componentManager->getSingleton<PhysicalDeviceComponent>().device;
			vk::Device device = componentManager->getSingleton<DeviceComponent>().device;
			vk::SurfaceKHR surface = componentManager->getSingleton<SurfaceComponent>().surface;
			vma::Allocator gpuAllocator = componentManager->getSingleton<GpuAllocatorComponent>().
			                                                allocator;
			SwapchainComponent swapchainComponent = componentManager->getSingleton<
				SwapchainComponent>();
			vk::SwapchainKHR swapchain = swapchainComponent.swapchain;
			vk::RenderPass renderPass = componentManager->getSingleton<RenderPassComponent>().
			                                              renderPass;
			FramebufferComponent& framebufferComponent = componentManager->getSingleton<
				FramebufferComponent>();
			CommandPoolComponent& commandPoolComponent = componentManager->getSingleton<
				CommandPoolComponent>();
			SynchronizationComponent& synchronizationComponent = componentManager->getSingleton<
				SynchronizationComponent>();

			auto queryBuilder = m_entityService->queryBuilder();
			Query1<ShaderComponent>& shaderQuery = *queryBuilder->buildQuery<ShaderComponent>();
			Query1<DescriptorSetLayoutComponent>& descriptorLayoutQuery = *queryBuilder->buildQuery<
				DescriptorSetLayoutComponent>();
			Query1<DescriptorPoolComponent>& descriptorPoolQuery = *queryBuilder->buildQuery<
				DescriptorPoolComponent>();
			Query1<PipelineLayoutComponent>& pipelineLayoutQuery = *queryBuilder->buildQuery<
				PipelineLayoutComponent>();
			Query1<PipelineComponent>& pipelineQuery = *queryBuilder->buildQuery<
				PipelineComponent>();

			Query1<UniformBufferComponent>& uniformBufferQuery = *queryBuilder->buildQuery<
				UniformBufferComponent>();

			SharedQuery1<UniformBufferSharedComponent>& uniformBufferSharedQuery = *queryBuilder->
				buildQuery<UniformBufferSharedComponent>();

			Query1<MeshComponent>& meshQuery = *queryBuilder->buildQuery<MeshComponent>();

			auto result = device.waitIdle();
			SASSERT_VULKAN(result)

			for (auto& mesh : meshQuery)
			{
				mesh.indexBuffer.destroy();
				mesh.vertexBuffer.destroy();
			}

			for (auto& ubo : uniformBufferQuery)
			{
				for (auto& buffer : ubo.ubos)
				{
					buffer.buffer.unmapMemory();
					buffer.buffer.destroy();
				}
			}

			for (auto& ubo : uniformBufferSharedQuery)
			{
				for (auto& buffer : ubo.ubos)
				{
					buffer.buffer.unmapMemory();
					buffer.buffer.destroy();
				}
			}

			for (auto& pipeline : pipelineQuery)
			{
				device.destroyPipeline(pipeline.pipeline, allocationCallbacks);
			}

			for (auto& pipelineLayout : pipelineLayoutQuery)
			{
				device.destroyPipelineLayout(pipelineLayout.layout, allocationCallbacks);
			}

			for (auto& pool : descriptorPoolQuery)
			{
				device.destroyDescriptorPool(pool.pool, allocationCallbacks);
			}

			for (auto& layout : descriptorLayoutQuery)
			{
				device.destroyDescriptorSetLayout(layout.layout, allocationCallbacks);
			}

			for (auto& shader : shaderQuery)
			{
				device.destroyShaderModule(shader.shaderModule, allocationCallbacks);
			}

			for (auto& semaphore : synchronizationComponent.imageAvailableSemaphores)
			{
				device.destroySemaphore(semaphore, allocationCallbacks);
			}
			for (auto& semaphore : synchronizationComponent.renderFinishedSemaphores)
			{
				device.destroySemaphore(semaphore, allocationCallbacks);
			}
			for (auto& fence : synchronizationComponent.inFlightFences)
			{
				device.destroyFence(fence, allocationCallbacks);
			}

			device.destroyCommandPool(commandPoolComponent.graphicsCommandPool,
			                          allocationCallbacks);
			device.destroyCommandPool(commandPoolComponent.transferCommandPool,
			                          allocationCallbacks);

			for (auto& framebuffer : framebufferComponent.framebuffers)
			{
				device.destroyFramebuffer(framebuffer, allocationCallbacks);
			}

			device.destroyRenderPass(renderPass, allocationCallbacks);

			device.destroySwapchainKHR(swapchain, allocationCallbacks);

			for (auto& imageView : swapchainComponent.imageViews)
			{
				device.destroyImageView(imageView, allocationCallbacks);
			}

			gpuAllocatorComponent.allocator.destroy();
			device.destroy(allocationCallbacks);

			destroyDebugUtilsMessenger(instance,
			                           componentManager->getSingleton<DebugMessengerComponent>().
			                                             messenger,
			                           nullptr);
			//instance.destroySurfaceKHR(surface,allocationCallbacks);
			instance.destroySurfaceKHR(surface);
			//instance.destroy();
			instance.destroy(allocationCallbacks);

			SDEBUG_LOG("CLEANUP COMPLETED")
			m_entityService->entityEventManager()->rewindEvent(typeid(CleanupRequest));
		}
	};
}
