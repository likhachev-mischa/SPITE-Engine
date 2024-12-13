#include "Base/VulkanUsage.hpp"
#include "GraphicsEngine.hpp"
#include "Application/AppConifg.hpp"
#include "GraphicsDebug.hpp"
#include "GraphicsUtility.hpp"
#include "Application/WindowManager.hpp"

#include <chrono>
#include <set>

#include "VulkanAllocator.hpp"
#include "Base/Logging.hpp"
#include "Base/Memory.hpp"


namespace spite
{
	GraphicsEngine::GraphicsEngine(spite::WindowManager* windowManager): m_heapAllocator("Vulkan Engine Allocator"),
	                                                                     m_windowManager(windowManager),
	                                                                     m_allocationCallbacks(
		                                                                     &m_heapAllocator, &vkAllocate,
		                                                                     &vkReallocate, &vkFree,
		                                                                     &vkAllocationCallback, &vkFreeCallback)
	{
		initVulkan();
	}

	size_t GraphicsEngine::selectedModelIdx()
	{
		return m_selectedModelIdx;
	}

	void GraphicsEngine::setSelectedModel(size_t idx)
	{
		m_selectedModelIdx = idx;
	}

	void GraphicsEngine::setUbo(const UniformBufferObject& ubo)
	{
		m_ubo.model = ubo.model;
		//	m_ubo.model[1][1] *= -1;
		m_ubo.color = ubo.color;
	}

	void GraphicsEngine::initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createVMAllocator();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPools();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}

	void GraphicsEngine::cleanup()
	{
		cleanupSwapChain();

		for (size_t i = 0; i < m_modelCount; ++i)
		{
			m_allocator.destroyBuffer(m_modelBuffers[i], m_modelBufferMemories[i]);
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_allocator.unmapMemory(m_uniformBuffersMemory[i]);
			m_allocator.destroyBuffer(m_uniformBuffers[i], m_uniformBuffersMemory[i]);
		}
		m_allocator.destroy();

		m_device.destroyDescriptorPool(m_descriptorPool);
		m_device.destroyDescriptorSetLayout(m_descriptorSetLayout);

		m_device.destroyPipeline(m_graphicsPipeline);
		m_device.destroyPipelineLayout(m_pipelineLayout);
		m_device.destroyRenderPass(m_renderPass);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_device.destroySemaphore(m_imageAvaliableSemaphores[i]);
			m_device.destroySemaphore(m_renderFinishedSemaphores[i]);
			m_device.destroyFence(m_inFlightFences[i]);
		}

		m_device.destroyCommandPool(m_graphicsCommandPool);
		m_device.destroyCommandPool(m_transferCommandPool);

		m_device.destroy();

		if (ENABLE_VALIDATION_LAYERS)
		{
			//destroyDebugUtilsMessengerExt(m_instance, m_debugMessenger, nullptr);
		}

		m_instance.destroySurfaceKHR(m_surface);
		m_instance.destroy(m_allocationCallbacks);

		m_heapAllocator.shutdown();
	}

	void GraphicsEngine::setModelData(const std::vector<const std::vector<glm::vec2>*>& vertices,
	                                  const std::vector<const std::vector<uint16_t>*>& indices)
	{
		for (size_t i = 0; i < m_modelCount; ++i)
		{
			m_allocator.destroyBuffer(m_modelBuffers[i], m_modelBufferMemories[i]);
		}

		m_modelCount = vertices.size();
		m_bufferData = new BufferData[m_modelCount];

		m_modelBuffers = new vk::Buffer[m_modelCount];
		m_modelBufferMemories = new vma::Allocation[m_modelCount];

		for (size_t i = 0; i < m_modelCount; ++i)
		{
			vk::DeviceSize bufferSize = 0;
			m_bufferData[i].verticesSize = (vertices[i])->size() * (sizeof(vertices[i]->operator[](0)));
			m_bufferData[i].indicesSize = (indices[i])->size() * (sizeof(indices[i]->operator[](0)));
			m_bufferData[i].indicesCount = indices[i]->size();
			bufferSize += m_bufferData[i].verticesSize + m_bufferData[i].indicesSize;

			vk::Buffer stagingBuffer;
			vma::Allocation stagingBufferAllocation;
			createBuffer(bufferSize,
			             vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
			             vk::BufferUsageFlagBits::eTransferSrc,
			             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
			             stagingBuffer,
			             stagingBufferAllocation,
			             vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
			             vma::AllocationCreateFlagBits::eStrategyMinTime);

			m_allocator.copyMemoryToAllocation(vertices[i]->data(),
			                                   stagingBufferAllocation,
			                                   0,
			                                   m_bufferData[i].verticesSize);
			m_allocator.copyMemoryToAllocation(indices[i]->data(),
			                                   stagingBufferAllocation,
			                                   m_bufferData[i].verticesSize,
			                                   m_bufferData[i].indicesSize);

			createBuffer(bufferSize,
			             vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
			             vk::BufferUsageFlagBits::eTransferDst,
			             vk::MemoryPropertyFlagBits::eDeviceLocal,
			             m_modelBuffers[i],
			             m_modelBufferMemories[i],
			             {});

			copyBuffer(stagingBuffer, m_modelBuffers[i], bufferSize);
			m_allocator.destroyBuffer(stagingBuffer, stagingBufferAllocation);
		}


		vk::DeviceSize offset = 0;
	}

	GraphicsEngine::~GraphicsEngine()
	{
		m_device.waitIdle();
		cleanup();
	}

	void GraphicsEngine::drawFrame()
	{
		if (m_device.waitForFences(1, &m_inFlightFences[m_currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to syncronize fences!");
		}

		uint32_t imageIndex;
		vk::Result result = m_device.acquireNextImageKHR(m_swapChain,
		                                                 UINT64_MAX,
		                                                 m_imageAvaliableSemaphores[m_currentFrame],
		                                                 {}, &imageIndex);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		m_device.resetFences({m_inFlightFences[m_currentFrame]});
		result = m_commandBuffers[m_currentFrame].reset();
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to reset command buffer!");
		}

		recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

		updateUniformBuffer(m_currentFrame);


		vk::Semaphore waitSemaphores[] = {m_imageAvaliableSemaphores[m_currentFrame]};
		vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
		vk::Semaphore signalSemaphores[] = {
			m_renderFinishedSemaphores[m_currentFrame]
		};


		vk::SubmitInfo submitInfo(1,
		                          waitSemaphores,
		                          waitStages,
		                          1,
		                          &m_commandBuffers[m_currentFrame],
		                          1,
		                          signalSemaphores);


		result = m_graphicsQueue.submit({submitInfo}, m_inFlightFences[m_currentFrame]);

		vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, &m_swapChain, &imageIndex, &result);
		result = m_presentQueue.presentKHR(presentInfo);

		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR ||
			framebufferResized)
		{
			framebufferResized = false;

			recreateSwapChain();
		}
		else if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void GraphicsEngine::createDescriptorSets()
	{
		std::vector<vk::DescriptorSetLayout> layouts(
			MAX_FRAMES_IN_FLIGHT,
			m_descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo(m_descriptorPool,
		                                        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
		                                        layouts.data());

		m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		vk::Result result;
		std::tie(result, m_descriptorSets) = m_device.allocateDescriptorSets(allocInfo);

		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vk::DescriptorBufferInfo bufferInfo(m_uniformBuffers[i], 0, sizeof(UniformBufferObject));

			vk::WriteDescriptorSet descriptorWrite(m_descriptorSets[i],
			                                       0,
			                                       0,
			                                       1,
			                                       vk::DescriptorType::eUniformBuffer,
			                                       {},
			                                       &bufferInfo,
			                                       {});

			m_device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
		}
	}

	void GraphicsEngine::createDescriptorPool()
	{
		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer,
		                                static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

		vk::DescriptorPoolCreateInfo poolInfo({}, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), 1, &poolSize);

		vk::Result result;
		std::tie(result, m_descriptorPool) = m_device.createDescriptorPool(poolInfo);

		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptor pool!");
		}
	}

	void GraphicsEngine::updateUniformBuffer(uint32_t currentImage)
	{
		// static auto startTime = std::chrono::high_resolution_clock::now();
		//
		// auto currentTime = std::chrono::high_resolution_clock::now();
		// float time = std::chrono::duration<float, std::chrono::seconds::period>(
		// 	currentTime - startTime).count();

		/*
		UniformBufferObject ubo{};
		ubo.model = (translation(0.1f * time, 0.1f * time));
		// ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
		//                        glm::vec3(0.0f, 0.0f, 0.0f),
		//                        glm::vec3(0.0f, 0.0f, 1.0f));
		// ubo.proj = glm::perspective(glm::radians(45.0f),
		//                             swapChainExtent.width / (float)
		//                             swapChainExtent.height,
		//                             0.1f,
		//                             10.f);
		//
		// ubo.proj[1][1] *= -1;
		//ubo.model = glm::mat3(1.0f);
		*/

		memcpy(m_uniformBuffersMapped[currentImage], &m_ubo, sizeof(m_ubo));
	}

	void GraphicsEngine::createVMAllocator()
	{
		vma::AllocatorCreateInfo createInfo({},
		                                    m_physicalDevice,
		                                    m_device,
		                                    {},
		                                    {},
		                                    {},
		                                    {},
		                                    {},
		                                    m_instance,
		                                    VK_API_VERSION);
		vk::Result result;
		std::tie(result, m_allocator) = vma::createAllocator(createInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create allocator!");
		}
	}

	void GraphicsEngine::createUniformBuffers()
	{
		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

		m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			createBuffer(bufferSize,
			             vk::BufferUsageFlagBits::eUniformBuffer,
			             vk::MemoryPropertyFlagBits::eHostVisible |
			             vk::MemoryPropertyFlagBits::eHostCoherent,
			             m_uniformBuffers[i],
			             m_uniformBuffersMemory[i],
			             vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);
			vk::Result result;
			std::tie(result, m_uniformBuffersMapped[i]) = m_allocator.mapMemory(m_uniformBuffersMemory[i]);
			if (result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to map uniform buffer memory!");
			}
		}
	}

	void GraphicsEngine::createDescriptorSetLayout()
	{
		vk::DescriptorSetLayoutBinding uboLayoutBinding(
			0,
			vk::DescriptorType::eUniformBuffer,
			1,
			vk::ShaderStageFlagBits::eVertex);

		vk::DescriptorSetLayoutCreateInfo layoutInfo({}, 1, &uboLayoutBinding);

		vk::Result result;
		std::tie(result, m_descriptorSetLayout) = m_device.createDescriptorSetLayout(layoutInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	void GraphicsEngine::recreateSwapChain()
	{
		int width = 0, height = 0;
		m_windowManager->getFramebufferSize(width, height);
		while (m_windowManager->isMinimized())
		{
			SDEBUG_LOG("Window is minimized, rendering halts!\n")
			m_windowManager->getFramebufferSize(width, height);
			m_windowManager->waitWindowExpand();
		}

		m_device.waitIdle();

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createFramebuffers();
	}

	void GraphicsEngine::cleanupSwapChain()
	{
		for (auto framebuffer : m_swapChainFramebuffers)
		{
			m_device.destroyFramebuffer(framebuffer, nullptr);
		}
		for (auto imageView : m_swapChainImageViews)
		{
			m_device.destroyImageView(imageView, nullptr);
		}

		m_device.destroySwapchainKHR(m_swapChain);
	}

	void GraphicsEngine::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo(m_transferCommandPool, vk::CommandBufferLevel::ePrimary, 1);

		std::vector<vk::CommandBuffer> commandBuffers;
		vk::Result result;
		std::tie(result, commandBuffers) = m_device.allocateCommandBuffers(allocInfo);

		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to allocate command buffer!");
		}

		vk::CommandBuffer commandBuffer = commandBuffers[0];

		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		result = commandBuffer.begin(beginInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to begin command buffer record!");
		}

		vk::BufferCopy copyRegion({}, {}, size);
		commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
		result = commandBuffer.end();
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to end command buffer record!");
		}

		vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer);

		result = m_transferQueue.submit({submitInfo});

		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to submit to transfer queue!");
		}
		result = m_transferQueue.waitIdle();
		m_device.freeCommandBuffers(m_transferCommandPool, 1, &commandBuffer);
	}

	void GraphicsEngine::createBuffer(vk::DeviceSize size,
	                                  vk::BufferUsageFlags usage,
	                                  vk::MemoryPropertyFlags properties,
	                                  vk::Buffer& buffer,
	                                  vma::Allocation& bufferMemory,
	                                  vma::AllocationCreateFlags allocationFlags)
	{
		QueueFaimilyIndices queueFaimilyIndices = findQueueFamilies(
			m_physicalDevice);
		uint32_t queues[] = {
			queueFaimilyIndices.graphicsFamily.value(),
			queueFaimilyIndices.transferFamily.value()
		};

		vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eConcurrent, 2, queues);

		vma::AllocationCreateInfo allocInfo(allocationFlags, vma::MemoryUsage::eAuto, properties);
		vk::Result result;
		std::pair<vk::Buffer, vma::Allocation> bufferAllocation;
		std::tie(result, bufferAllocation) = m_allocator.createBuffer(bufferInfo, allocInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create buffer!");
		}

		buffer = bufferAllocation.first;
		bufferMemory = bufferAllocation.second;
	}

	uint32_t GraphicsEngine::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
	{
		vk::PhysicalDeviceMemoryProperties memProperties = m_physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
		{
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].
				propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	void GraphicsEngine::createSyncObjects()
	{
		m_imageAvaliableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		vk::SemaphoreCreateInfo semaphoreInfo;

		vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_renderFinishedSemaphores[i] = m_device.createSemaphore(semaphoreInfo).value;
			m_imageAvaliableSemaphores[i] = m_device.createSemaphore(semaphoreInfo).value;
			m_inFlightFences[i] = m_device.createFence(fenceInfo).value;
		}
	}

	void GraphicsEngine::createCommandBuffers()
	{
		m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		vk::CommandBufferAllocateInfo allocInfo(m_graphicsCommandPool,
		                                        vk::CommandBufferLevel::ePrimary,
		                                        static_cast<uint32_t>(m_commandBuffers.size()));

		vk::Result result;
		std::tie(result, m_commandBuffers) = m_device.allocateCommandBuffers(allocInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create command buffer!");
		}
	}

	void GraphicsEngine::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
	{
		vk::CommandBufferBeginInfo beginInfo;

		commandBuffer.begin(beginInfo);

		vk::Rect2D renderArea({}, m_swapChainExtent);
		vk::ClearValue clearColor({0.0f, 0.0f, 0.0f, 1.0f});
		vk::RenderPassBeginInfo renderPassInfo(m_renderPass,
		                                       m_swapChainFramebuffers[imageIndex],
		                                       renderArea,
		                                       1,
		                                       &clearColor);

		commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);

		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(m_swapChainExtent.width),
		                      static_cast<float>(m_swapChainExtent.height),
		                      0.0f,
		                      1.0f);
		commandBuffer.setViewport(0, 1, &viewport);


		commandBuffer.setScissor(0, 1, &renderArea);

		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &m_modelBuffers[m_selectedModelIdx], &offset);

		commandBuffer.bindIndexBuffer(m_modelBuffers[m_selectedModelIdx], m_bufferData[m_selectedModelIdx].verticesSize,
		                              vk::IndexType::eUint16);


		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                                 m_pipelineLayout,
		                                 0,
		                                 1,
		                                 &m_descriptorSets[m_currentFrame],
		                                 {},
		                                 {});

		commandBuffer.drawIndexed(m_bufferData[m_selectedModelIdx].indicesCount, 1, 0, 0, 0);
		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

	void GraphicsEngine::createCommandPools()
	{
		QueueFaimilyIndices queueFaimilyIndices = findQueueFamilies(
			m_physicalDevice);

		vk::CommandPoolCreateInfo graphicPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		                                                queueFaimilyIndices.graphicsFamily.value());

		vk::Result result;
		std::tie(result, m_graphicsCommandPool) = m_device.createCommandPool(graphicPoolCreateInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create graphics command pool!");
		}

		vk::CommandPoolCreateInfo transferPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient,
		                                                 queueFaimilyIndices.transferFamily.value());

		std::tie(result, m_transferCommandPool) = m_device.createCommandPool(transferPoolCreateInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create transfer command pool!");
		}
	}

	void GraphicsEngine::createFramebuffers()
	{
		m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

		for (size_t i = 0; i < m_swapChainImageViews.size(); ++i)
		{
			vk::ImageView attachments[] = {m_swapChainImageViews[i]};

			vk::FramebufferCreateInfo framebufferInfo({},
			                                          m_renderPass,
			                                          1,
			                                          attachments,
			                                          m_swapChainExtent.width,
			                                          m_swapChainExtent.height,
			                                          1);

			vk::Result result;
			std::tie(result, m_swapChainFramebuffers[i]) = m_device.createFramebuffer(framebufferInfo);
			if (result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	void GraphicsEngine::createRenderPass()
	{
		vk::AttachmentDescription colorAttachment(
			{},
			m_swapChainImageFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR);

		vk::AttachmentReference colorAttachmentRef(
			0,
			vk::ImageLayout::eColorAttachmentOptimal);


		vk::SubpassDescription supbpass({},
		                                vk::PipelineBindPoint::eGraphics,
		                                {},
		                                {},
		                                1,
		                                &colorAttachmentRef);

		vk::RenderPassCreateInfo renderPassInfo({},
		                                        1,
		                                        &colorAttachment,
		                                        1,
		                                        &supbpass);

		vk::SubpassDependency dependency(vk::SubpassExternal,
		                                 0,
		                                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                 {},
		                                 vk::AccessFlagBits::eColorAttachmentWrite);

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		vk::Result result;
		std::tie(result, m_renderPass) = m_device.createRenderPass(renderPassInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	void GraphicsEngine::createGraphicsPipeline()
	{
		auto vertShaderCode = readBinaryFile("shaders/vert.spv");
		auto fragShaderCode = readBinaryFile("shaders/frag.spv");

		vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		vertShaderCode.clear();
		fragShaderCode.clear();

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
			{},
			vk::ShaderStageFlagBits::eVertex,
			vertShaderModule,
			"main");

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
			{},
			vk::ShaderStageFlagBits::eFragment,
			fragShaderModule,
			"main");

		vk::PipelineShaderStageCreateInfo shaderStages[] = {
			vertShaderStageInfo, fragShaderStageInfo
		};

		std::vector<vk::DynamicState> dynamicStates = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
			vk::DynamicState::eLineWidth
		};

		vk::PipelineDynamicStateCreateInfo dynamicState(
			{},
			static_cast<uint32_t>(dynamicStates.size()),
			dynamicStates.data());

		vk::VertexInputBindingDescription bindingDescription(0, sizeof(glm::vec2), vk::VertexInputRate::eVertex);
		vk::VertexInputAttributeDescription attributeDescription(0, 0, vk::Format::eR32G32Sfloat);
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
			{},
			1,
			&bindingDescription,
			1,
			&attributeDescription);


		vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
			{},
			vk::PrimitiveTopology::eTriangleList,
			vk::False);

		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(m_swapChainExtent.width),
		                      static_cast<float>(m_swapChainExtent.height),
		                      0.0f,
		                      1.0f);

		vk::Rect2D scissor({}, m_swapChainExtent);

		vk::PipelineViewportStateCreateInfo viewportState(
			{},
			1,
			&viewport,
			1,
			&scissor);

		vk::PipelineRasterizationStateCreateInfo rasterizer({},
		                                                    vk::False,
		                                                    vk::False,
		                                                    vk::PolygonMode::eFill,
		                                                    vk::CullModeFlagBits::eBack,
		                                                    vk::FrontFace::eClockwise,
		                                                    vk::False);

		vk::PipelineMultisampleStateCreateInfo multisampling(
			{},
			vk::SampleCountFlagBits::e1,
			vk::False);

		vk::PipelineColorBlendAttachmentState colorBlendAttachment(
			vk::False,
			{},
			{},
			{},
			{},
			{},
			{},
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);

		vk::PipelineColorBlendStateCreateInfo colorBlending(
			{},
			vk::False,
			{},
			1,
			&colorBlendAttachment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
			{},
			1,
			&m_descriptorSetLayout);

		vk::Result result;
		struct GLFWwindow;
		std::tie(result, m_pipelineLayout) = m_device.createPipelineLayout(pipelineLayoutInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		vk::GraphicsPipelineCreateInfo pipelineInfo({},
		                                            2,
		                                            shaderStages,
		                                            &vertexInputInfo,
		                                            &inputAssembly,
		                                            {},
		                                            &viewportState,
		                                            &rasterizer,
		                                            &multisampling,
		                                            {},
		                                            &colorBlending,
		                                            &dynamicState,
		                                            m_pipelineLayout,
		                                            m_renderPass,
		                                            0);

		m_graphicsPipeline = m_device.createGraphicsPipeline(
			VK_NULL_HANDLE,
			pipelineInfo).value;

		m_device.destroyShaderModule(vertShaderModule, nullptr);
		m_device.destroyShaderModule(fragShaderModule, nullptr);
	}

	void GraphicsEngine::createImageViews()
	{
		m_swapChainImageViews.resize(m_swapChainImages.size());

		for (size_t i = 0; i < m_swapChainImages.size(); ++i)
		{
			vk::ImageSubresourceRange subresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0,
				1,
				0,
				1);
			vk::ImageViewCreateInfo createInfo({},
			                                   m_swapChainImages[i],
			                                   vk::ImageViewType::e2D,
			                                   m_swapChainImageFormat,
			                                   {},
			                                   subresourceRange);
			vk::Result result;
			std::tie(result, m_swapChainImageViews[i]) = m_device.createImageView(createInfo);
			if (result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create image view!");
			}
		}
	}

	vk::ShaderModule GraphicsEngine::createShaderModule(const std::vector<char>& code)
	{
		vk::ShaderModuleCreateInfo createInfo({},
		                                      code.size(),
		                                      reinterpret_cast<const uint32_t*>(
			                                      code.data()));

		vk::ShaderModule shaderModule;
		vk::Result result;
		std::tie(result, shaderModule) = m_device.createShaderModule(createInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create shader module!");
		}
		return shaderModule;
	}

	void GraphicsEngine::createSurface()
	{
		m_surface = m_windowManager->createWindowSurface(m_instance);
	}

	void GraphicsEngine::createInstance()
	{
		if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
		{
			throw std::runtime_error(
				"Validation layers requested, but not available!");
		}

		vk::ApplicationInfo appInfo(APPLICATION_NAME,
		                            vk::makeApiVersion(0, 1, 0, 0),
		                            ENGINE_NAME,
		                            vk::makeApiVersion(0, 1, 0, 0));

		auto extensions = getRequiredExtensions();
		vk::InstanceCreateInfo createInfo({},
		                                  &appInfo,
		                                  {},
		                                  {},
		                                  static_cast<uint32_t>(extensions.size()),
		                                  extensions.data(),
		                                  {});


		if (ENABLE_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(
				VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo =
				createDebugMessengerCreateInfo();
			createInfo.pNext = &debugCreateInfo;
		}

		vk::Result result;
		std::tie(result, m_instance) = vk::createInstance(createInfo, m_allocationCallbacks);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create instance!");
		}
	}

	void GraphicsEngine::createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(
			m_physicalDevice);

		vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(
			swapChainSupport.formats);
		vk::PresentModeKHR presentMode = chooseSwapPresentMode(
			swapChainSupport.presentModes);
		vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount >
			swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		vk::SwapchainCreateInfoKHR createInfo({},
		                                      m_surface,
		                                      imageCount,
		                                      surfaceFormat.format,
		                                      surfaceFormat.colorSpace,
		                                      extent,
		                                      1,
		                                      vk::ImageUsageFlagBits::eColorAttachment,
		                                      {},
		                                      {},
		                                      {},
		                                      swapChainSupport.capabilities.currentTransform,
		                                      vk::CompositeAlphaFlagBitsKHR::eOpaque,
		                                      presentMode,
		                                      vk::True);


		QueueFaimilyIndices indices = findQueueFamilies(m_physicalDevice);
		uint32_t queueFamilyInidces[] = {
			indices.graphicsFamily.value(), indices.presentFamily.value()
		};

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyInidces;
		}
		else
		{
			createInfo.imageSharingMode = vk::SharingMode::eExclusive;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		vk::Result result;
		std::tie(result, m_swapChain) = m_device.createSwapchainKHR(createInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create swapchan!");
		}

		std::tie(result, m_swapChainImages) = m_device.getSwapchainImagesKHR(m_swapChain);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to get swapchain images!");
		}

		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;
	}

	void GraphicsEngine::pickPhysicalDevice()
	{
		std::vector<vk::PhysicalDevice> devices;
		vk::Result result;
		std::tie(result, devices) = m_instance.enumeratePhysicalDevices();
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to enumerate physical devices!");
		}

		if (devices.empty())
		{
			throw std::runtime_error("Failed to locate physical devices!");
		}

		for (const auto& device : devices)
		{
			if (device)
			{
				m_physicalDevice = device;
				break;
			}
		}

		if (m_physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	void GraphicsEngine::createLogicalDevice()
	{
		QueueFaimilyIndices indices = findQueueFamilies(m_physicalDevice);

		std::set<uint32_t> uniqueQueueFamilies = {
			indices.graphicsFamily.value(), indices.presentFamily.value(),
			indices.transferFamily.value()
		};

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		queueCreateInfos.reserve(uniqueQueueFamilies.size());

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			vk::DeviceQueueCreateInfo queueCreateInfo(
				{},
				queueFamily,
				1,
				&queuePriority,
				{});
			queueCreateInfos.emplace_back(queueCreateInfo);
		}

		vk::PhysicalDeviceFeatures deviceFeatueres{};

		vk::DeviceCreateInfo createInfo({},
		                                static_cast<uint32_t>(queueCreateInfos.size()),
		                                queueCreateInfos.data(),
		                                {},
		                                {},
		                                static_cast<uint32_t>(DEVICE_EXTENSIONS.size()),
		                                DEVICE_EXTENSIONS.data(),
		                                &deviceFeatueres);

		if (ENABLE_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(
				VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		}

		vk::Result result;
		std::tie(result, m_device) = m_physicalDevice.createDevice(createInfo);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to creaete physical device!");
		}

		m_graphicsQueue = m_device.getQueue(indices.graphicsFamily.value(), 0);
		m_presentQueue = m_device.getQueue(indices.presentFamily.value(), 0);
		m_transferQueue = m_device.getQueue(indices.transferFamily.value(), 0);
	}

	bool GraphicsEngine::isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFaimilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(
				device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !
				swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	bool GraphicsEngine::checkDeviceExtensionSupport(vk::PhysicalDevice device)
	{
		std::vector<vk::ExtensionProperties> avaliableExtensions;
		vk::Result result;
		std::tie(result, avaliableExtensions) = device.enumerateDeviceExtensionProperties();
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to enumerate device extension properties!");
		}

		std::set<std::string> requiredExtensions(
			DEVICE_EXTENSIONS.begin(),
			DEVICE_EXTENSIONS.end());

		for (const auto& extension : avaliableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	GraphicsEngine::SwapChainSupportDetails GraphicsEngine::querySwapChainSupport(vk::PhysicalDevice device)
	{
		SwapChainSupportDetails details;

		details.capabilities = device.getSurfaceCapabilitiesKHR(m_surface).value;
		details.formats = device.getSurfaceFormatsKHR(m_surface).value;
		details.presentModes = device.getSurfacePresentModesKHR(m_surface).value;

		return details;
	}

	vk::SurfaceFormatKHR GraphicsEngine::chooseSwapSurfaceFormat(
		const std::vector<vk::SurfaceFormatKHR>& avaliableFormats)
	{
		for (const auto& avaliableFormat : avaliableFormats)
		{
			if (avaliableFormat.format == vk::Format::eB8G8R8A8Srgb &&
				avaliableFormat.colorSpace ==
				vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return avaliableFormat;
			}
		}
		return avaliableFormats[0];
	}

	vk::PresentModeKHR GraphicsEngine::chooseSwapPresentMode(
		const std::vector<vk::PresentModeKHR>& avaliablePresentModes)
	{
		for (const auto& avaliablePresentMode : avaliablePresentModes)
		{
			if (avaliablePresentMode == vk::PresentModeKHR::eMailbox)
			{
				return avaliablePresentMode;
			}
		}

		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D GraphicsEngine::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
#undef max
		if (capabilities.currentExtent.width != std::numeric_limits<
			uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		int width, height;
		m_windowManager->getFramebufferSize(width, height);

		vk::Extent2D actualExtent(static_cast<uint32_t>(width),
		                          static_cast<uint32_t>(height));

		actualExtent.width = std::clamp(actualExtent.width,
		                                capabilities.minImageExtent.width,
		                                capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height,
		                                 capabilities.minImageExtent.height,
		                                 capabilities.maxImageExtent.height);
		return actualExtent;
	}

	bool GraphicsEngine::QueueFaimilyIndices::isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value() &&
			transferFamily.has_value();
	}

	GraphicsEngine::QueueFaimilyIndices GraphicsEngine::findQueueFamilies(vk::PhysicalDevice device)
	{
		QueueFaimilyIndices indices;

		std::vector<vk::QueueFamilyProperties> queueFamilies = device.
			getQueueFamilyProperties();

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.graphicsFamily = i;
			}
			else if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
			{
				indices.transferFamily = i;
			}

			vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, m_surface).value;
			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}
			i++;
		}

		return indices;
	}

	void GraphicsEngine::setupDebugMessenger()
	{
		if (!ENABLE_VALIDATION_LAYERS) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo =
			createDebugMessengerCreateInfo();

		if (createDebugUtilsMessengerExt(m_instance,
		                                 &createInfo,
		                                 nullptr,
		                                 &m_debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create debug messenger!");
		}
	}

	std::vector<const char*> GraphicsEngine::getRequiredExtensions()
	{
		uint32_t extensionCount = 0;
		char const* const* extensionNames;
		extensionNames = m_windowManager->getExtensions(extensionCount);

		std::vector<const char*> extensions(extensionNames,
		                                    extensionNames +
		                                    extensionCount);

		if (ENABLE_VALIDATION_LAYERS)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool GraphicsEngine::checkValidationLayerSupport()
	{
		std::vector<vk::LayerProperties> availableLayers =
			vk::enumerateInstanceLayerProperties().value;

		for (const char* layerName : VALIDATION_LAYERS)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}
}
