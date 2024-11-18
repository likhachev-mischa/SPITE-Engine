#pragma once
#include <vulkan-memory-allocator-hpp/vk_mem_alloc.hpp>
#include <glm/glm.hpp>
#include <optional>

#include "Base/Memory.hpp"

namespace spite
{
	class WindowManager;

	const std::array<const char*, 1> DEVICE_EXTENSIONS = {
		vk::KHRSwapchainExtensionName
	};

	struct BufferData
	{
		vk::DeviceSize verticesSize;
		vk::DeviceSize indicesSize;
		size_t indicesCount;
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::vec4 color;
		//glm::mat4 view;
		//glm::mat4 proj;
	};


	class GraphicsEngine
	{
	public:
		bool framebufferResized = false;

		GraphicsEngine(spite::WindowManager* windowManager);

		size_t selectedModelIdx();

		void setSelectedModel(size_t idx);

		void setUbo(const UniformBufferObject& ubo);

		void setModelData(const std::vector<const std::vector<glm::vec2>*>& vertices,
		                  const std::vector<const std::vector<uint16_t>*>& indices);
		void drawFrame();

		~GraphicsEngine();

	private:
		spite::HeapAllocator m_heapAllocator;
		vk::AllocationCallbacks m_allocationCallbacks;

		UniformBufferObject m_ubo;

		BufferData* m_bufferData;
		size_t m_selectedModelIdx = 0;
		size_t m_modelCount = 0;

		vk::Buffer* m_modelBuffers;
		vma::Allocation* m_modelBufferMemories;

		spite::WindowManager* m_windowManager;

		vk::Instance m_instance;

		vk::SurfaceKHR m_surface;

		vk::SwapchainKHR m_swapChain;
		std::vector<vk::Image> m_swapChainImages;
		vk::Format m_swapChainImageFormat;
		vk::Extent2D m_swapChainExtent;

		std::vector<vk::Framebuffer> m_swapChainFramebuffers;
		std::vector<vk::ImageView> m_swapChainImageViews;

		vk::RenderPass m_renderPass;
		vk::DescriptorSetLayout m_descriptorSetLayout;
		vk::PipelineLayout m_pipelineLayout;
		vk::Pipeline m_graphicsPipeline;

		VkDebugUtilsMessengerEXT m_debugMessenger;

		vk::Device m_device;
		vk::PhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

		vk::Queue m_graphicsQueue;
		vk::Queue m_presentQueue;
		vk::Queue m_transferQueue;

		vk::CommandPool m_graphicsCommandPool;
		vk::CommandPool m_transferCommandPool;
		std::vector<vk::CommandBuffer> m_commandBuffers;

		std::vector<vk::Semaphore> m_imageAvaliableSemaphores;
		std::vector<vk::Semaphore> m_renderFinishedSemaphores;
		std::vector<vk::Fence> m_inFlightFences;

		std::vector<vk::Buffer> m_uniformBuffers;
		std::vector<vma::Allocation> m_uniformBuffersMemory;
		std::vector<void*> m_uniformBuffersMapped;

		vk::DescriptorPool m_descriptorPool;
		std::vector<vk::DescriptorSet> m_descriptorSets;

		uint32_t m_currentFrame = 0;

		vma::Allocator m_allocator;

		void initVulkan();

		void cleanup();

		void createDescriptorSets();

		void createDescriptorPool();

		void updateUniformBuffer(uint32_t currentImage);

		void createVMAllocator();

		void createUniformBuffers();

		void createDescriptorSetLayout();

		void recreateSwapChain();

		void cleanupSwapChain();

		void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

		void createBuffer(vk::DeviceSize size,
		                  vk::BufferUsageFlags usage,
		                  vk::MemoryPropertyFlags properties,
		                  vk::Buffer& buffer,
		                  vma::Allocation& bufferMemory,
		                  vma::AllocationCreateFlags allocationFlags);

		uint32_t findMemoryType(uint32_t typeFilter,
		                        vk::MemoryPropertyFlags properties);

		void createSyncObjects();

		void createCommandBuffers();

		void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

		void createCommandPools();

		void createFramebuffers();

		void createRenderPass();

		void createGraphicsPipeline();

		void createImageViews();

		vk::ShaderModule createShaderModule(const std::vector<char>& code);

		void createSurface();

		void createInstance();

		void createSwapChain();

		void pickPhysicalDevice();

		void createLogicalDevice();

		bool isDeviceSuitable(VkPhysicalDevice device);

		bool checkDeviceExtensionSupport(vk::PhysicalDevice device);

		struct SwapChainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> presentModes;
		};

		SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

		vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
			const std::vector<vk::SurfaceFormatKHR>& avaliableFormats);

		vk::PresentModeKHR chooseSwapPresentMode(
			const std::vector<vk::PresentModeKHR>& avaliablePresentModes);

		vk::Extent2D chooseSwapExtent(
			const vk::SurfaceCapabilitiesKHR& capabilities);

		struct QueueFaimilyIndices
		{
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;
			std::optional<uint32_t> transferFamily;

			bool isComplete();
		};

		QueueFaimilyIndices findQueueFamilies(vk::PhysicalDevice device);

		void setupDebugMessenger();

		std::vector<const char*> getRequiredExtensions();

		bool checkValidationLayerSupport();
	};
}
