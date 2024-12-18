#include "Modules.hpp"

#include "Application/WindowManager.hpp"

#include "Base/Assert.hpp"
#include "Base/File.hpp"
#include "Base/Logging.hpp"

#include "Engine/RenderingCore.hpp"
#include "Engine/ResourcesCore.hpp"


namespace spite
{
	CoreModule::CoreModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
	                       char const* const* windowExtensions, const u32 extensionCount,
	                       const spite::HeapAllocator& allocator):
		allocationCallbacks(std::move(allocationCallbacksPtr)),
		extensions(windowExtensions, extensionCount, allocator),
		instanceWrapper(allocator, *allocationCallbacks, extensions),
		physicalDeviceWrapper(instanceWrapper),
		debugMessengerWrapper(instanceWrapper, *allocationCallbacks)
	{
	}

	BaseModule::BaseModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
	                       std::shared_ptr<CoreModule> coreModulePtr, const spite::HeapAllocator& allocator,
	                       const vk::SurfaceKHR& surface):
		allocationCallbacks(std::move(allocationCallbacksPtr)),
		coreModule(std::move(coreModulePtr)),
		surface(surface),
		indices(spite::findQueueFamilies(surface, coreModule->physicalDeviceWrapper.device, allocator)),
		deviceWrapper(coreModule->physicalDeviceWrapper, indices, allocator, *allocationCallbacks),
		gpuAllocatorWrapper(coreModule->physicalDeviceWrapper, deviceWrapper, coreModule->instanceWrapper,
		                    *allocationCallbacks),
		transferCommandPool(deviceWrapper, indices.transferFamily.value(),
		                    vk::CommandPoolCreateFlagBits::eTransient,
		                    *allocationCallbacks)
	{
		vk::Device device = deviceWrapper.device;
		transferQueue = device.getQueue(indices.transferFamily.value(), 0);
		presentQueue = device.getQueue(indices.presentFamily.value(), 0);
		graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
	}

	BaseModule::~BaseModule()
	{
		coreModule->instanceWrapper.instance.destroySurfaceKHR(surface, nullptr);
	}

	SwapchainModule::SwapchainModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
	                                 std::shared_ptr<CoreModule> coreModulePtr,
	                                 std::shared_ptr<BaseModule> baseModulePtr,
	                                 const spite::HeapAllocator& allocator,
	                                 const int width, const int height):
		allocationCallbacks(std::move(allocationCallbacksPtr)),
		coreModule(std::move(coreModulePtr)),
		baseModule(std::move(baseModulePtr)),
		swapchainDetailsWrapper(coreModule->physicalDeviceWrapper, baseModule->surface, width, height),
		swapchainWrapper(baseModule->deviceWrapper, baseModule->indices, swapchainDetailsWrapper,
		                 baseModule->surface,
		                 *allocationCallbacks),
		renderPassWrapper(baseModule->deviceWrapper, swapchainDetailsWrapper,
		                  *allocationCallbacks),
		swapchainImagesWrapper(baseModule->deviceWrapper, swapchainWrapper),
		imageViewsWrapper(baseModule->deviceWrapper, swapchainImagesWrapper, swapchainDetailsWrapper, allocator,
		                  *allocationCallbacks),
		framebuffersWrapper(baseModule->deviceWrapper, allocator, imageViewsWrapper, swapchainDetailsWrapper,
		                    renderPassWrapper, *allocationCallbacks)
	{
	}

	void SwapchainModule::recreate(const int width, const int height)
	{
		swapchainDetailsWrapper = SwapchainDetailsWrapper(coreModule->physicalDeviceWrapper, baseModule->surface,
		                                                  width, height);
		swapchainWrapper.recreate(swapchainDetailsWrapper);
		renderPassWrapper.recreate(swapchainDetailsWrapper);

		swapchainImagesWrapper = SwapchainImagesWrapper(baseModule->deviceWrapper, swapchainWrapper);

		imageViewsWrapper.recreate(swapchainImagesWrapper, swapchainDetailsWrapper);
		framebuffersWrapper.recreate(swapchainDetailsWrapper, imageViewsWrapper, renderPassWrapper);
	}

	DescriptorModule::DescriptorModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
	                                   std::shared_ptr<BaseModule> baseModulePtr, const vk::DescriptorType& type,
	                                   const u32 count, const BufferWrapper& bufferWrapper,
	                                   const sizet bufferElementSize,
	                                   const spite::HeapAllocator& allocator):
		allocationCallbacks(std::move(allocationCallbacksPtr)),
		baseModule(std::move(baseModulePtr)),
		descriptorSetLayoutWrapper(baseModule->deviceWrapper, type, *allocationCallbacks),
		descriptorPoolWrapper(baseModule->deviceWrapper, type, count,
		                      *allocationCallbacks),
		descriptorSetsWrapper(baseModule->deviceWrapper, descriptorSetLayoutWrapper, descriptorPoolWrapper,
		                      allocator, *allocationCallbacks, count, bufferWrapper, bufferElementSize)
	{
	}

	ShaderServiceModule::ShaderServiceModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
	                                         std::shared_ptr<BaseModule> baseModulePtr,
	                                         const spite::HeapAllocator& allocator):
		allocationCallbacks(std::move(allocationCallbacksPtr)),
		baseModule(std::move(baseModulePtr)),
		shaderModules(allocator),
		bufferAllocator(allocator)
	{
	}

	ShaderModuleWrapper& ShaderServiceModule::getShaderModule(const char* shaderPath,
	                                                          const vk::ShaderStageFlagBits& bits)
	{
		eastl::string shaderPathStr(shaderPath);

		auto it = shaderModules.find(shaderPathStr);
		if (it == shaderModules.end())
		{
			shaderModules.emplace(
				shaderPathStr, ShaderModuleWrapper(baseModule->deviceWrapper,
				                                   readBinaryFile(shaderPath, bufferAllocator),
				                                   bits, *allocationCallbacks));
			return shaderModules.at(shaderPathStr);
		}
		return it->second;
	}

	void ShaderServiceModule::removeShaderModule(const char* shaderPath)
	{
		eastl::string shaderPathStr(shaderPath);
		if (shaderModules.find(shaderPathStr) != shaderModules.end())
		{
			shaderModules.erase(shaderPathStr);
		}
	}

	ShaderServiceModule::~ShaderServiceModule()
	{
		shaderModules.clear();
	}

	GraphicsCommandModule::GraphicsCommandModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
	                                             std::shared_ptr<BaseModule> baseModulePtr,
	                                             const vk::CommandPoolCreateFlagBits& flagBits, const u32 count):
		allocationCallbacks(std::move(allocationCallbacksPtr)),
		baseModule(std::move(baseModulePtr)),
		commandPoolWrapper(baseModule->deviceWrapper, baseModule->indices.graphicsFamily.value(), flagBits,
		                   *allocationCallbacks),
		primaryCommandBuffersWrapper(baseModule->deviceWrapper, commandPoolWrapper, vk::CommandBufferLevel::ePrimary,
		                             count),
		secondaryCommandBuffersWrapper(baseModule->deviceWrapper, commandPoolWrapper,
		                               vk::CommandBufferLevel::eSecondary, count)
	{
	}

	ModelDataModule::ModelDataModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
	                                 std::shared_ptr<BaseModule> baseModulePtr,
	                                 const eastl::vector<glm::vec3, spite::HeapAllocator>& vertices,
	                                 const eastl::vector<u32, spite::HeapAllocator>& indices): allocationCallbacks(
			std::move(allocationCallbacksPtr)),
		baseModule(std::move(baseModulePtr)),
		indicesCount(static_cast<u32>(indices.size()))
	{
		vertSize = vertices.size() * sizeof(vertices[0]);
		sizet indSize = indicesCount * sizeof(indices[0]);
		sizet totalSize = vertSize + indSize;

		modelBuffer = BufferWrapper(
			totalSize,
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
			vk::BufferUsageFlagBits::eTransferDst,
			vk::MemoryPropertyFlagBits::eDeviceLocal, {}, baseModule->indices, baseModule->gpuAllocatorWrapper);

		auto stagingBuffer = BufferWrapper(
			totalSize,
			vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eVertexBuffer |
			vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eStrategyMinTime,
			baseModule->indices, baseModule->gpuAllocatorWrapper);

		stagingBuffer.copyMemory(vertices.data(), vertSize, 0);
		stagingBuffer.copyMemory(indices.data(), indSize, vertSize);

		modelBuffer.copyBuffer(stagingBuffer, baseModule->deviceWrapper.device,
		                       baseModule->transferCommandPool.commandPool,
		                       baseModule->transferQueue,
		                       &allocationCallbacks->allocationCallbacks);
	}

	UboModule::UboModule(const std::shared_ptr<CoreModule>& coreModulePtr,
	                     const std::shared_ptr<BaseModule>& baseModulePtr,
	                     const sizet elementSize, const sizet elementCount)
	{
		sizet minUboAlignment = coreModulePtr->physicalDeviceWrapper.device.getProperties().limits.
		                                       minUniformBufferOffsetAlignment;
		sizet dynamicAlignment = (elementSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		elementAlignment = dynamicAlignment;

		uboBuffer = BufferWrapper(elementCount * elementAlignment, vk::BufferUsageFlagBits::eUniformBuffer,
		                          vk::MemoryPropertyFlagBits::eHostVisible |
		                          vk::MemoryPropertyFlagBits::eHostCoherent,
		                          vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, baseModulePtr->indices,
		                          baseModulePtr->gpuAllocatorWrapper);
		memory = uboBuffer.mapMemory();
	}

	UboModule::~UboModule()
	{
		uboBuffer.unmapMemory();
	}

	RenderModule::RenderModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
	                           std::shared_ptr<BaseModule> baseModulePtr,
	                           std::shared_ptr<SwapchainModule> swapchainModulePtr,
	                           std::shared_ptr<DescriptorModule> descriptorModulePtr,
	                           std::shared_ptr<GraphicsCommandModule> commandBuffersModulePtr,
	                           eastl::vector<std::shared_ptr<ModelDataModule>, spite::HeapAllocator> models,
	                           const spite::HeapAllocator& allocator,
	                           const eastl::vector<
		                           eastl::tuple<ShaderModuleWrapper&, const char*>, spite::HeapAllocator>&
	                           shaderModules,
	                           const VertexInputDescriptionsWrapper& vertexInputDescriptions,
	                           std::shared_ptr<WindowManager> windowManagerPtr, const u32 framesInFlight):
		windowManager(std::move(windowManagerPtr)),
		allocationCallbacks(std::move(allocationCallbacksPtr)),
		baseModule(std::move(baseModulePtr)), swapchainModule(std::move(swapchainModulePtr)),
		descriptorModule(std::move(descriptorModulePtr)),
		commandBuffersModule(std::move(commandBuffersModulePtr)),
		models(std::move(models)),
		graphicsPipelineWrapper(baseModule->deviceWrapper, descriptorModule->descriptorSetLayoutWrapper,
		                        swapchainModule->swapchainDetailsWrapper,
		                        swapchainModule->renderPassWrapper, allocator, shaderModules,
		                        vertexInputDescriptions, *allocationCallbacks),
		syncObjectsWrapper(baseModule->deviceWrapper, framesInFlight, *allocationCallbacks),
		currentFrame(0)
	{
	}

	void RenderModule::waitForFrame()
	{
		vk::Result result = spite::waitForFrame(baseModule->deviceWrapper.device,
		                                        swapchainModule->swapchainWrapper.swapchain,
		                                        syncObjectsWrapper.inFlightFences[currentFrame],
		                                        syncObjectsWrapper.imageAvailableSemaphores[currentFrame],
		                                        imageIndex);
		recreateSwapchain(result);
	}

	void RenderModule::drawFrame()
	{
		beginSecondaryCommandBuffer(commandBuffersModule->secondaryCommandBuffersWrapper.commandBuffers[currentFrame],
		                            swapchainModule->renderPassWrapper.renderPass,
		                            swapchainModule->framebuffersWrapper.framebuffers[imageIndex]);


		for (sizet i = 0, size = models.size(); i < size; ++i)
		{
			u32 dynamicOffset = i * descriptorModule->descriptorSetsWrapper.dynamicOffset;
			std::shared_ptr<ModelDataModule> model = models[i];
			recordSecondaryCommandBuffer(
				commandBuffersModule->secondaryCommandBuffersWrapper.commandBuffers[currentFrame],
				graphicsPipelineWrapper.graphicsPipeline, graphicsPipelineWrapper.pipelineLayout,
				descriptorModule->descriptorSetsWrapper.descriptorSets[currentFrame],
				swapchainModule->swapchainDetailsWrapper.extent,
				model->modelBuffer.buffer, dynamicOffset, model->vertSize, model->indicesCount);
		}

		endSecondaryCommandBuffer(commandBuffersModule->secondaryCommandBuffersWrapper.commandBuffers[currentFrame]);

		recordPrimaryCommandBuffer(commandBuffersModule->primaryCommandBuffersWrapper.commandBuffers[currentFrame],
		                           swapchainModule->swapchainDetailsWrapper.extent,
		                           swapchainModule->renderPassWrapper.renderPass,
		                           swapchainModule->framebuffersWrapper.framebuffers[imageIndex],
		                           commandBuffersModule->secondaryCommandBuffersWrapper.commandBuffers[currentFrame]);

		vk::Result result = spite::drawFrame(
			commandBuffersModule->primaryCommandBuffersWrapper.commandBuffers[currentFrame],
			syncObjectsWrapper.inFlightFences[currentFrame],
			syncObjectsWrapper.imageAvailableSemaphores[currentFrame],
			syncObjectsWrapper.renderFinishedSemaphores[currentFrame],
			baseModule->graphicsQueue,
			baseModule->presentQueue,
			swapchainModule->swapchainWrapper.swapchain,
			imageIndex);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		recreateSwapchain(result);
	}

	RenderModule::~RenderModule()
	{
		vk::Result result = baseModule->deviceWrapper.device.waitIdle();
		SASSERT_VULKAN(result);
	}

	void RenderModule::recreateSwapchain(const vk::Result result)
	{
		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
		{
			while (windowManager->isMinimized())
			{
				windowManager->waitWindowExpand();
			}
			int width = 0, height = 0;
			windowManager->getFramebufferSize(width, height);

			vk::Result waitResult = baseModule->deviceWrapper.device.waitIdle();
			SASSERT_VULKAN(waitResult);
			swapchainModule->recreate(width, height);
			graphicsPipelineWrapper.recreate(swapchainModule->swapchainDetailsWrapper,
			                                 swapchainModule->renderPassWrapper);
		}
		else
		{
			SASSERT_VULKAN(result);
		}
	}
}
