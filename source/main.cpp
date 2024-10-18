#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define VMA_IMPLEMENTATION

//#include <vma/vk_mem_alloc.h>
#include <vk_mem_alloc.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <optional>
#include <set>

#include <cstdint>
#include <limits>
#include <algorithm>
#include <array>
#include <fstream>
#include <chrono>

#include <vulkan/vulkan.hpp>

//TODO: compile and import std module


const uint32_t VK_API_VERSION = VK_API_VERSION_1_3;

const char APPLICATION_NAME[] = "Application";
const char ENGINE_NAME[] = "ENGINE";

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		vk::VertexInputBindingDescription bindingDescription(
			0,
			sizeof(Vertex),
			vk::VertexInputRate::eVertex);

		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 2>
	getAttributeDescriptions()
	{
		std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions
			{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

class Application;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::array<const char*, 1> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::array<const char*, 1> deviceExtensions = {
	vk::KHRSwapchainExtensionName
};

static std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}


#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT*
                                      pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance,
		"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}


static void
framebufferResizeCallback(GLFWwindow* window, int width, int height);

//TODO: change allocation to vma lib
//TODO: change multiple buffers to offset buffer
//TODO: rewrite to cpp bindings
class Application
{
public:
	bool framebufferResized = false;

	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window;

	vk::Instance instance;

	vk::SurfaceKHR surface;

	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;

	std::vector<vk::Framebuffer> swapChainFramebuffers;
	std::vector<vk::ImageView> swapChainImageViews;

	vk::RenderPass renderPass;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;

	VkDebugUtilsMessengerEXT debugMessenger;

	vk::Device device;
	vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;

	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::Queue transferQueue;

	vk::CommandPool graphicsCommandPool;
	vk::CommandPool transferCommandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Semaphore> imageAvaliableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;

	vk::Buffer vertexBuffer;
	vma::Allocation vertexBufferMemory;

	vk::Buffer indexBuffer;
	vma::Allocation indexBufferMemory;

	std::vector<vk::Buffer> uniformBuffers;
	std::vector<vma::Allocation> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	vk::DescriptorPool descriptorPool;
	std::vector<vk::DescriptorSet> descriptorSets;

	uint32_t currentFrame = 0;

	vma::Allocator allocator;

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}


	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createAllocator();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPools();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}

		device.waitIdle();
	}

	void cleanup()
	{
		cleanupSwapChain();

		allocator.destroyBuffer(vertexBuffer, vertexBufferMemory);

		allocator.destroyBuffer(indexBuffer, indexBufferMemory);
		// vkDestroyBuffer(device, indexBuffer, nullptr);
		//vkFreeMemory(device, indexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			//vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			//vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
			allocator.unmapMemory(uniformBuffersMemory[i]);
			allocator.destroyBuffer(uniformBuffers[i], uniformBuffersMemory[i]);
		}
		allocator.destroy();

		device.destroyDescriptorPool(descriptorPool);
		device.destroyDescriptorSetLayout(descriptorSetLayout);

		device.destroyPipeline(graphicsPipeline);
		device.destroyPipelineLayout(pipelineLayout);
		device.destroyRenderPass(renderPass);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			device.destroySemaphore(imageAvaliableSemaphores[i]);
			device.destroySemaphore(renderFinishedSemaphores[i]);
			device.destroyFence(inFlightFences[i]);
		}

		device.destroyCommandPool(graphicsCommandPool);
		device.destroyCommandPool(transferCommandPool);

		device.destroy();

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		instance.destroySurfaceKHR(surface);
		instance.destroy();

		glfwDestroyWindow(window);

		glfwTerminate();
	}


	void drawFrame()
	{
		if (device.waitForFences(1, &inFlightFences[currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to syncronize fences!");
		}

		uint32_t imageIndex;
		vk::Result result = device.acquireNextImageKHR(swapChain,
		                                               UINT64_MAX,
		                                               imageAvaliableSemaphores[currentFrame],
		                                               {},
		                                               &imageIndex);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		device.resetFences({inFlightFences[currentFrame]});
		commandBuffers[currentFrame].reset();
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		updateUniformBuffer(currentFrame);


		vk::Semaphore waitSemaphores[] = {imageAvaliableSemaphores[currentFrame]};
		vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
		vk::Semaphore signalSemaphores[] = {
			renderFinishedSemaphores[currentFrame]
		};


		vk::SubmitInfo submitInfo(1,
		                          waitSemaphores,
		                          waitStages,
		                          1,
		                          &commandBuffers[currentFrame],
		                          1,
		                          signalSemaphores);


		graphicsQueue.submit({submitInfo}, inFlightFences[currentFrame]);

		vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, &swapChain, &imageIndex, &result);
		result = presentQueue.presentKHR(presentInfo);

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

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void createDescriptorSets()
	{
		std::vector<vk::DescriptorSetLayout> layouts(
			MAX_FRAMES_IN_FLIGHT,
			descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo(descriptorPool,
		                                        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
		                                        layouts.data());

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		descriptorSets = device.allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vk::DescriptorBufferInfo bufferInfo(uniformBuffers[i], 0, sizeof(UniformBufferObject));

			vk::WriteDescriptorSet descriptorWrite(descriptorSets[i],
			                                       0,
			                                       0,
			                                       1,
			                                       vk::DescriptorType::eUniformBuffer,
			                                       {},
			                                       &bufferInfo,
			                                       {});

			device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
		}
	}

	void createDescriptorPool()
	{
		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer,
		                                static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

		vk::DescriptorPoolCreateInfo poolInfo({}, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), 1, &poolSize);

		descriptorPool = device.createDescriptorPool(poolInfo);
	}

	void updateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(
			currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f),
		                        time * glm::radians(90.0f),
		                        glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
		                       glm::vec3(0.0f, 0.0f, 0.0f),
		                       glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f),
		                            swapChainExtent.width / (float)
		                            swapChainExtent.height,
		                            0.1f,
		                            10.f);

		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}


	void createAllocator()
	{
		vma::AllocatorCreateInfo createInfo({},
		                                    physicalDevice,
		                                    device,
		                                    {},
		                                    {},
		                                    {},
		                                    {},
		                                    {},
		                                    instance,
		                                    VK_API_VERSION);
		allocator = vma::createAllocator(createInfo);
	}

	void createUniformBuffers()
	{
		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			createBuffer(bufferSize,
			             vk::BufferUsageFlagBits::eUniformBuffer,
			             vk::MemoryPropertyFlagBits::eHostVisible |
			             vk::MemoryPropertyFlagBits::eHostCoherent,
			             uniformBuffers[i],
			             uniformBuffersMemory[i],
			             vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);
			uniformBuffersMapped[i] = allocator.mapMemory(uniformBuffersMemory[i]);
			//vkMapMemory(device,
			//          uniformBuffersMemory[i],
			//        0,
			//      bufferSize,
			//    0,
			//  &uniformBuffersMapped[i]);
		}
	}

	void createDescriptorSetLayout()
	{
		vk::DescriptorSetLayoutBinding uboLayoutBinding(
			0,
			vk::DescriptorType::eUniformBuffer,
			1,
			vk::ShaderStageFlagBits::eVertex);

		vk::DescriptorSetLayoutCreateInfo layoutInfo({}, 1, &uboLayoutBinding);

		descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
	}

	void createIndexBuffer()
	{
		vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		vk::Buffer stagingBuffer;
		vma::Allocation stagingBufferMemory;
		createBuffer(bufferSize,
		             vk::BufferUsageFlagBits::eTransferSrc,
		             vk::MemoryPropertyFlagBits::eHostVisible |
		             vk::MemoryPropertyFlagBits::eHostCoherent,
		             stagingBuffer,
		             stagingBufferMemory,
		             vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

		allocator.copyMemoryToAllocation(indices.data(), stagingBufferMemory, {}, bufferSize);
		//void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize, {});
		//memcpy(data, indices.data(), (size_t)bufferSize);
		//device.unmapMemory(stagingBufferMemory);

		createBuffer(bufferSize,
		             vk::BufferUsageFlagBits::eTransferDst |
		             vk::BufferUsageFlagBits::eIndexBuffer,
		             vk::MemoryPropertyFlagBits::eDeviceLocal,
		             indexBuffer,
		             indexBufferMemory,
		             {});

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		allocator.destroyBuffer(stagingBuffer, stagingBufferMemory);
		//	vkDestroyBuffer(device, stagingBuffer, nullptr);
		//	vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		device.waitIdle();

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createFramebuffers();
	}

	void cleanupSwapChain()
	{
		for (auto framebuffer : swapChainFramebuffers)
		{
			device.destroyFramebuffer(framebuffer, nullptr);
		}
		for (auto imageView : swapChainImageViews)
		{
			device.destroyImageView(imageView, nullptr);
		}

		device.destroySwapchainKHR(swapChain);
	}

	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo(transferCommandPool, vk::CommandBufferLevel::ePrimary, 1);

		vk::CommandBuffer commandBuffer;
		commandBuffer = device.allocateCommandBuffers(allocInfo)[0];

		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(beginInfo);

		vk::BufferCopy copyRegion({}, {}, size);
		commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
		commandBuffer.end();

		vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer);

		transferQueue.submit({submitInfo});
		//vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
		transferQueue.waitIdle();
		//vkQueueWaitIdle(transferQueue);
		device.freeCommandBuffers(transferCommandPool, 1, &commandBuffer);

		//vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);
	}

	void createVertexBuffer()
	{
		vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		vk::Buffer stagingBuffer;
		//	vk::DeviceMemory stagingBufferMemory;
		vma::Allocation stagingBufferMemory;

		createBuffer(bufferSize,
		             vk::BufferUsageFlagBits::eTransferSrc,
		             vk::MemoryPropertyFlagBits::eHostVisible |
		             vk::MemoryPropertyFlagBits::eHostCoherent,
		             stagingBuffer,
		             stagingBufferMemory,
		             vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

		//void* data = allocator.mapMemory(stagingBufferMemory);
		//memcpy(data, vertices.data(), (size_t)bufferSize);
		//allocator.unmapMemory(stagingBufferMemory);
		allocator.copyMemoryToAllocation(vertices.data(), stagingBufferMemory, {}, bufferSize);

		createBuffer(bufferSize,
		             vk::BufferUsageFlagBits::eTransferDst |
		             vk::BufferUsageFlagBits::eVertexBuffer,
		             vk::MemoryPropertyFlagBits::eDeviceLocal,
		             vertexBuffer,
		             vertexBufferMemory,
		             {});

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		allocator.destroyBuffer(stagingBuffer, stagingBufferMemory);
		//	vkDestroyBuffer(device, stagingBuffer, nullptr);
		//	vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createBuffer(vk::DeviceSize size,
	                  vk::BufferUsageFlags usage,
	                  vk::MemoryPropertyFlags properties,
	                  vk::Buffer& buffer,
	                  //vk::DeviceMemory& bufferMemory)
	                  vma::Allocation& bufferMemory,
	                  vma::AllocationCreateFlags allocationFlags)
	{
		QueueFaimilyIndices queueFaimilyIndices = findQueueFamilies(
			physicalDevice);
		uint32_t queues[] = {
			queueFaimilyIndices.graphicsFamily.value(),
			queueFaimilyIndices.transferFamily.value()
		};

		vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eConcurrent, 2, queues);

		//buffer = device.createBuffer(bufferInfo);

		//vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);

		//	vk::MemoryAllocateInfo allocInfo(memRequirements.size,
		//                                 findMemoryType(memRequirements.memoryTypeBits, properties));

		vma::AllocationCreateInfo allocInfo(allocationFlags, vma::MemoryUsage::eAuto, properties);
		std::pair<vk::Buffer, vma::Allocation> bufferAllocation = allocator.createBuffer(bufferInfo, allocInfo);
		buffer = bufferAllocation.first;
		bufferMemory = bufferAllocation.second;

		//	bufferMemory = device.allocateMemory(allocInfo);

		//	device.bindBufferMemory(buffer, bufferMemory, 0);
	}

	uint32_t findMemoryType(uint32_t typeFilter,
	                        vk::MemoryPropertyFlags properties)
	{
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

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

	void createSyncObjects()
	{
		imageAvaliableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		vk::SemaphoreCreateInfo semaphoreInfo;

		vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
			imageAvaliableSemaphores[i] = device.createSemaphore(semaphoreInfo);
			inFlightFences[i] = device.createFence(fenceInfo);
		}
	}

	void createCommandBuffers()
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		vk::CommandBufferAllocateInfo allocInfo(graphicsCommandPool,
		                                        vk::CommandBufferLevel::ePrimary,
		                                        static_cast<uint32_t>(commandBuffers.size()));

		commandBuffers = device.allocateCommandBuffers(allocInfo);
	}

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
	{
		vk::CommandBufferBeginInfo beginInfo;

		commandBuffer.begin(beginInfo);

		vk::Rect2D renderArea({}, swapChainExtent);
		vk::ClearValue clearColor({0.0f, 0.0f, 0.0f, 1.0f});
		vk::RenderPassBeginInfo renderPassInfo(renderPass,
		                                       swapChainFramebuffers[imageIndex],
		                                       renderArea,
		                                       1,
		                                       &clearColor);

		commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(swapChainExtent.width),
		                      static_cast<float>(swapChainExtent.height),
		                      0.0f,
		                      1.0f);
		commandBuffer.setViewport(0, 1, &viewport);


		commandBuffer.setScissor(0, 1, &renderArea);

		vk::Buffer vertexBuffers[] = {vertexBuffer};
		vk::DeviceSize offsets[] = {0};
		commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

		commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                                 pipelineLayout,
		                                 0,
		                                 1,
		                                 &descriptorSets[currentFrame],
		                                 {},
		                                 {});

		commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

	void createCommandPools()
	{
		QueueFaimilyIndices queueFaimilyIndices = findQueueFamilies(
			physicalDevice);

		vk::CommandPoolCreateInfo graphicPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		                                                queueFaimilyIndices.graphicsFamily.value());

		graphicsCommandPool = device.createCommandPool(graphicPoolCreateInfo);

		vk::CommandPoolCreateInfo transferPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient,
		                                                 queueFaimilyIndices.transferFamily.value());

		transferCommandPool = device.createCommandPool(transferPoolCreateInfo);
	}

	void createFramebuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImageViews.size(); ++i)
		{
			vk::ImageView attachments[] = {swapChainImageViews[i]};

			vk::FramebufferCreateInfo framebufferInfo({},
			                                          renderPass,
			                                          1,
			                                          attachments,
			                                          swapChainExtent.width,
			                                          swapChainExtent.height,
			                                          1);

			swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
		}
	}

	void createRenderPass()
	{
		vk::AttachmentDescription colorAttachment(
			{},
			swapChainImageFormat,
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

		renderPass = device.createRenderPass(renderPassInfo);
	}

	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

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


		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescription = Vertex::getAttributeDescriptions();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
			{},
			1,
			&bindingDescription,
			static_cast<uint32_t>(attributeDescription.size()),
			attributeDescription.data());

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
			{},
			vk::PrimitiveTopology::eTriangleList,
			vk::False);

		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(swapChainExtent.width),
		                      static_cast<float>(swapChainExtent.height),
		                      0.0f,
		                      1.0f);

		vk::Rect2D scissor({}, swapChainExtent);

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
		                                                    vk::FrontFace::eCounterClockwise,
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
			&descriptorSetLayout);

		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

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
		                                            pipelineLayout,
		                                            renderPass,
		                                            0);

		graphicsPipeline = device.createGraphicsPipeline(
			VK_NULL_HANDLE,
			pipelineInfo).value;

		device.destroyShaderModule(vertShaderModule, nullptr);
		device.destroyShaderModule(fragShaderModule, nullptr);
	}

	void createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); ++i)
		{
			vk::ImageSubresourceRange subresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0,
				1,
				0,
				1);
			vk::ImageViewCreateInfo createInfo({},
			                                   swapChainImages[i],
			                                   vk::ImageViewType::e2D,
			                                   swapChainImageFormat,
			                                   {},
			                                   subresourceRange);
			swapChainImageViews[i] = device.createImageView(createInfo);
		}
	}

	vk::ShaderModule createShaderModule(const std::vector<char>& code)
	{
		vk::ShaderModuleCreateInfo createInfo({},
		                                      code.size(),
		                                      reinterpret_cast<const uint32_t*>(
			                                      code.data()));

		vk::ShaderModule shaderModule;
		shaderModule = device.createShaderModule(createInfo);
		return shaderModule;
	}

	void createSurface()
	{
		VkSurfaceKHR tempSurface;
		if (glfwCreateWindowSurface(instance, window, nullptr, &tempSurface) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create window surface!");
		}
		surface = tempSurface;
	}

	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
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


		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(
				validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo =
				createDebugMessengerCreateInfo();
			createInfo.pNext = &debugCreateInfo;
		}

		instance = vk::createInstance(createInfo, nullptr);
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(
			physicalDevice);

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
		                                      surface,
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


		QueueFaimilyIndices indices = findQueueFamilies(physicalDevice);
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

		swapChain = device.createSwapchainKHR(createInfo);

		swapChainImages = device.getSwapchainImagesKHR(swapChain);

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void pickPhysicalDevice()
	{
		std::vector<vk::PhysicalDevice> devices = instance.
			enumeratePhysicalDevices();

		if (devices.empty())
		{
			throw std::runtime_error("Failed to locate physical devices!");
		}

		for (const auto& device : devices)
		{
			if (device)
			{
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	void createLogicalDevice()
	{
		QueueFaimilyIndices indices = findQueueFamilies(physicalDevice);

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
		                                static_cast<uint32_t>(deviceExtensions.size()),
		                                deviceExtensions.data(),
		                                &deviceFeatueres);

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(
				validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}

		device = physicalDevice.createDevice(createInfo);

		graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
		presentQueue = device.getQueue(indices.presentFamily.value(), 0);
		transferQueue = device.getQueue(indices.transferFamily.value(), 0);
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
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

	bool checkDeviceExtensionSupport(vk::PhysicalDevice device)
	{
		std::vector<vk::ExtensionProperties> avaliableExtensions = device.
			enumerateDeviceExtensionProperties();
		std::set<std::string> requiredExtensions(
			deviceExtensions.begin(),
			deviceExtensions.end());

		for (const auto& extension : avaliableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	vk::DebugUtilsMessengerCreateInfoEXT createDebugMessengerCreateInfo()
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo({},
		                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		                                                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		                                                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		                                                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		                                                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		                                                debugCallback);
		return createInfo;
	}

	struct SwapChainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device)
	{
		SwapChainSupportDetails details;

		details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
		details.formats = device.getSurfaceFormatsKHR(surface);
		details.presentModes = device.getSurfacePresentModesKHR(surface);

		return details;
	}

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
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

	vk::PresentModeKHR chooseSwapPresentMode(
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

	vk::Extent2D chooseSwapExtent(
		const vk::SurfaceCapabilitiesKHR& capabilities)
	{
#undef max
		if (capabilities.currentExtent.width != std::numeric_limits<
			uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

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

	struct QueueFaimilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> transferFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value() &&
				transferFamily.has_value();
		}
	};

	QueueFaimilyIndices findQueueFamilies(vk::PhysicalDevice device)
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

			vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface);
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

	void setupDebugMessenger()
	{
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo =
			createDebugMessengerCreateInfo();

		if (CreateDebugUtilsMessengerEXT(instance,
		                                 &createInfo,
		                                 nullptr,
		                                 &debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create debug messenger!");
		}
	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions,
		                                    glfwExtensions +
		                                    glfwExtensionCount);

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkValidationLayerSupport()
	{
		std::vector<vk::LayerProperties> availableLayers =
			vk::enumerateInstanceLayerProperties();

		for (const char* layerName : validationLayers)
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

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage <<
			std::endl;

		return VK_FALSE;
	}
};

int main()
{
	Application app;
	app.run();
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}
