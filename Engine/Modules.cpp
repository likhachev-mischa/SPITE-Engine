#include "Modules.hpp"

#include "Application/WindowManager.hpp"

#include "Base/Assert.hpp"
#include "Base/File.hpp"
#include "Base/Logging.hpp"

#include "Engine/RenderingCore.hpp"
#include "Engine/ResourcesCore.hpp"


namespace spite
{
	//TODO: REMOVE WINDOWMANAGER
	BaseModule::BaseModule(spite::HeapAllocator& allocator, char const* const* windowExtensions,
	                       const u32 windowExtensionCount, spite::WindowManager* windowManager):
		allocationCallbacksWrapper(allocator), extensions(windowExtensions, windowExtensionCount, allocator),
		instanceWrapper(allocator, allocationCallbacksWrapper, extensions),
		surface(windowManager->createWindowSurface(instanceWrapper.instance)),
		physicalDeviceWrapper(instanceWrapper),
		indices(spite::findQueueFamilies(surface, physicalDeviceWrapper.device, allocator)),
		deviceWrapper(physicalDeviceWrapper, indices, allocator, allocationCallbacksWrapper),
		gpuAllocatorWrapper(physicalDeviceWrapper, deviceWrapper, instanceWrapper, allocationCallbacksWrapper),
		transferCommandPool(deviceWrapper, indices.transferFamily.value(), vk::CommandPoolCreateFlagBits::eTransient,
		                    allocationCallbacksWrapper),
		debugMessengerWrapper(instanceWrapper, allocationCallbacksWrapper)
	{
		vk::Device device = deviceWrapper.device;
		transferQueue = device.getQueue(indices.transferFamily.value(), 0);
		presentQueue = device.getQueue(indices.presentFamily.value(), 0);
		graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
	}

	SwapchainModule::SwapchainModule(std::shared_ptr<BaseModule> baseModulePtr, const spite::HeapAllocator& allocator,
	                                 const int width, const int height):
		baseModule(std::move(baseModulePtr)),
		swapchainDetailsWrapper(baseModule->physicalDeviceWrapper, baseModule->surface, width, height),
		swapchainWrapper(baseModule->deviceWrapper, baseModule->indices, swapchainDetailsWrapper,
		                 baseModule->surface,
		                 baseModule->allocationCallbacksWrapper),
		renderPassWrapper(baseModule->deviceWrapper, swapchainDetailsWrapper,
		                  baseModule->allocationCallbacksWrapper),
		swapchainImagesWrapper(baseModule->deviceWrapper, swapchainWrapper),
		imageViewsWrapper(baseModule->deviceWrapper, swapchainImagesWrapper, swapchainDetailsWrapper,
		                  baseModule->allocationCallbacksWrapper),
		framebuffersWrapper(baseModule->deviceWrapper, allocator, imageViewsWrapper, swapchainDetailsWrapper,
		                    renderPassWrapper, baseModule->allocationCallbacksWrapper)
	{
	}

	void SwapchainModule::recreate(const int width, const int height)
	{
		swapchainDetailsWrapper = SwapchainDetailsWrapper(baseModule->physicalDeviceWrapper, baseModule->surface,
		                                                  width, height);
		swapchainWrapper.recreate(swapchainDetailsWrapper);
		renderPassWrapper.recreate(swapchainDetailsWrapper);

		swapchainImagesWrapper = SwapchainImagesWrapper(baseModule->deviceWrapper, swapchainWrapper);

		imageViewsWrapper.recreate(swapchainImagesWrapper, swapchainDetailsWrapper);
		framebuffersWrapper.recreate(swapchainDetailsWrapper, imageViewsWrapper, renderPassWrapper);
	}

	DescriptorModule::DescriptorModule(std::shared_ptr<BaseModule> baseModulePtr, const vk::DescriptorType& type,
	                                   const u32 size, const BufferWrapper& bufferWrapper,
	                                   const sizet bufferElementSize,
	                                   const spite::HeapAllocator& allocator):
		baseModule(std::move(baseModulePtr)),
		descriptorSetLayoutWrapper(baseModule->deviceWrapper, type, baseModule->allocationCallbacksWrapper),
		descriptorPoolWrapper(baseModule->deviceWrapper, type, size,
		                      baseModule->allocationCallbacksWrapper),
		descriptorSetsWrapper(baseModule->deviceWrapper, descriptorSetLayoutWrapper, descriptorPoolWrapper,
		                      allocator, baseModule->allocationCallbacksWrapper, size, bufferWrapper, bufferElementSize)
	{
	}

	ShaderServiceModule::ShaderServiceModule(std::shared_ptr<BaseModule> baseModulePtr,
	                                         const spite::HeapAllocator& allocator):
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
				                                   bits, baseModule->allocationCallbacksWrapper));
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

	GraphicsCommandModule::GraphicsCommandModule(std::shared_ptr<BaseModule> baseModulePtr,
	                                             const vk::CommandPoolCreateFlagBits& flagBits, const u32 count):
		baseModule(std::move(baseModulePtr)),
		commandPoolWrapper(baseModule->deviceWrapper, baseModule->indices.graphicsFamily.value(), flagBits,
		                   baseModule->allocationCallbacksWrapper),
		primaryCommandBuffersWrapper(baseModule->deviceWrapper, commandPoolWrapper, vk::CommandBufferLevel::ePrimary,
		                             count),
		secondaryCommandBuffersWrapper(baseModule->deviceWrapper, commandPoolWrapper,
		                               vk::CommandBufferLevel::eSecondary, count)
	{
	}

	ModelDataModule::ModelDataModule(std::shared_ptr<BaseModule> baseModulePtr,
	                                 const eastl::vector<glm::vec3>& vertices,
	                                 const eastl::vector<u32>& indices): indicesCount(static_cast<u32>(indices.size())),
	                                                                     baseModule(std::move(baseModulePtr))
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
		                       &baseModule->allocationCallbacksWrapper.allocationCallbacks);
	}

	UboModule::UboModule(std::shared_ptr<BaseModule> baseModulePtr,
	                     const sizet elementSize, const sizet elementCount): baseModule(std::move(baseModulePtr))
	{
		sizet minUboAlignment = baseModule->physicalDeviceWrapper.device.getProperties().limits.
		                                    minUniformBufferOffsetAlignment;
		sizet dynamicAlignment = (elementSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		elementAlignment = dynamicAlignment;

		uboBuffer = BufferWrapper(elementCount * elementSize, vk::BufferUsageFlagBits::eUniformBuffer,
		                          vk::MemoryPropertyFlagBits::eHostVisible |
		                          vk::MemoryPropertyFlagBits::eHostCoherent,
		                          vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, baseModule->indices,
		                          baseModule->gpuAllocatorWrapper);
		memory = uboBuffer.mapMemory();
	}

	UboModule::~UboModule()
	{
		uboBuffer.unmapMemory();
	}

	RenderModule::RenderModule(std::shared_ptr<BaseModule> baseModulePtr,
	                           std::shared_ptr<SwapchainModule> swapchainModulePtr,
	                           std::shared_ptr<DescriptorModule> descriptorModulePtr,
	                           std::shared_ptr<GraphicsCommandModule> commandBuffersModulePtr,
	                           eastl::vector<std::shared_ptr<ModelDataModule>> models,
	                           const spite::HeapAllocator& allocator,
	                           const eastl::vector<
		                           eastl::tuple<ShaderModuleWrapper&, const char*>, spite::HeapAllocator>&
	                           shaderModules,
	                           const VertexInputDescriptionsWrapper& vertexInputDescriptions,
	                           WindowManager* windowManager, const u32 framesInFlight):
		baseModule(std::move(baseModulePtr)),
		swapchainModule(std::move(swapchainModulePtr)), descriptorModule(std::move(descriptorModulePtr)),
		commandBuffersModule(std::move(commandBuffersModulePtr)),
		models(std::move(models)),
		graphicsPipelineWrapper(baseModule->deviceWrapper, descriptorModule->descriptorSetLayoutWrapper,
		                        swapchainModule->swapchainDetailsWrapper,
		                        swapchainModule->renderPassWrapper, allocator, shaderModules,
		                        vertexInputDescriptions, baseModule->allocationCallbacksWrapper),
		syncObjectsWrapper(baseModule->deviceWrapper, framesInFlight, baseModule->allocationCallbacksWrapper),
		currentFrame(0),
		m_windowManager(windowManager)
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
			recordSecondaryCommandBuffer(
				commandBuffersModule->secondaryCommandBuffersWrapper.commandBuffers[currentFrame],
				graphicsPipelineWrapper.graphicsPipeline, graphicsPipelineWrapper.pipelineLayout,
				descriptorModule->descriptorSetsWrapper.descriptorSets[currentFrame],
				swapchainModule->swapchainDetailsWrapper.extent,
				models[i]->modelBuffer.buffer, models[i]->vertSize, models[i]->indicesCount);
		}

		endSecondaryCommandBuffer(commandBuffersModule->secondaryCommandBuffersWrapper.commandBuffers[currentFrame]);

		recordPrimaryCommandBuffer(commandBuffersModule->primaryCommandBuffersWrapper.commandBuffers[currentFrame],
		                           swapchainModule->swapchainDetailsWrapper.extent,
		                           swapchainModule->renderPassWrapper.renderPass,
		                           swapchainModule->framebuffersWrapper.framebuffers[imageIndex],
		                           graphicsPipelineWrapper.graphicsPipeline,
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

		recreateSwapchain(result);
	}

	void RenderModule::recreateSwapchain(const vk::Result result)
	{
		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
		{
			while (m_windowManager->isMinimized())
			{
				m_windowManager->waitWindowExpand();
			}
			int width = 0, height = 0;
			m_windowManager->getFramebufferSize(width, height);

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
