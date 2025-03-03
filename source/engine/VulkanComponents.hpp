#pragma once
#include <glm/fwd.hpp>
#include <glm/detail/type_quat.hpp>

#include "ecs/Core.hpp"

#include "base/VmaUsage.hpp"
#include "Engine/Common.hpp"
#include "Engine/Resources.hpp"

namespace spite
{
	// Core Vulkan infrastructure components
	struct VulkanInstanceComponent : ISingletonComponent
	{
		vk::Instance instance;
		std::vector<const char*> enabledExtensions;
		bool validationLayersEnabled = false;
	};

	struct PhysicalDeviceComponent : ISingletonComponent
	{
		vk::PhysicalDevice device;
		vk::PhysicalDeviceProperties properties;
		vk::PhysicalDeviceFeatures features;
		vk::PhysicalDeviceMemoryProperties memoryProperties;
	};

	struct LogicalDeviceComponent : ISingletonComponent
	{
		vk::Device device;
		QueueFamilyIndices queueFamilyIndices;
	};

	struct QueueComponent : ISingletonComponent
	{
		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
		vk::Queue transferQueue;
		uint32_t graphicsQueueIndex;
		uint32_t presentQueueIndex;
		uint32_t transferQueueIndex;
	};

	struct SurfaceComponent : ISingletonComponent
	{
		vk::SurfaceKHR surface;
		vk::SurfaceCapabilitiesKHR capabilities;
	};

	struct SwapchainComponent : ISingletonComponent
	{
		vk::SwapchainKHR swapchain;
		vk::Format imageFormat;
		vk::ColorSpaceKHR colorSpace;
		vk::Extent2D extent;
		std::vector<vk::Image> images;
		std::vector<vk::ImageView> imageViews;
	};

	struct RenderPassComponent : IComponent
	{
		vk::RenderPass renderPass;
	};

	struct FramebufferComponent : IComponent
	{
		std::vector<vk::Framebuffer> framebuffers;
	};

	struct AllocationCallbacksComponent : ISingletonComponent
	{
		vk::AllocationCallbacks* allocationCallbacks;
	};

	struct GpuAllocatorComponent : ISingletonComponent
	{
		vma::Allocator allocator;
	};

	//Resource components

	struct CommandPoolComponent : IComponent {
		vk::CommandPool graphicsCommandPool;
		vk::CommandPool transferCommandPool;
	};

	struct CommandBufferComponent : IComponent {
		std::vector<vk::CommandBuffer> primaryBuffers;
		std::vector<vk::CommandBuffer> secondaryBuffers;
		uint32_t currentBufferIndex = 0;
	};

	struct SynchronizationComponent : IComponent {
		std::vector<vk::Semaphore> imageAvailableSemaphores;
		std::vector<vk::Semaphore> renderFinishedSemaphores;
		std::vector<vk::Fence> inFlightFences;
		uint32_t maxFramesInFlight = 2;
		uint32_t currentFrame = 0;
	};

	struct ShaderComponent : IComponent {
		vk::ShaderModule shaderModule;
		vk::ShaderStageFlagBits stage;
		std::string entryPoint = "main";
		std::string filePath;  // For hot reloading
	};

	struct DescriptorSetLayoutComponent : IComponent {
		vk::DescriptorSetLayout layout;
		vk::DescriptorType type;
		uint32_t bindingIndex;
		vk::ShaderStageFlags stages;
	};

	struct DescriptorPoolComponent : IComponent {
		vk::DescriptorPool pool;
		uint32_t maxSets;
	};

	struct DescriptorSetsComponent : IComponent {
		std::vector<vk::DescriptorSet> descriptorSets;
		uint32_t dynamicOffset = 0;
	};

	struct PipelineLayoutComponent : IComponent {
		vk::PipelineLayout layout;
		std::vector<Entity> descriptorSetLayoutEntities;
	};

	struct PipelineComponent : IComponent {
		vk::Pipeline pipeline;
		Entity pipelineLayoutEntity;
		VertexInputDescriptions vertexInputDescription;

		PipelineComponent():vertexInputDescription({},{}) {  }
	};

	struct MeshComponent : IComponent {
		BufferWrapper vertexBuffer;
		BufferWrapper indexBuffer;
		uint32_t vertexCount;
		uint32_t indexCount;
		vk::DeviceSize vertexBufferOffset = 0;
		bool isDirty = false;  // For dynamic meshes
	};

	struct MaterialComponent : IComponent {
		Entity shaderProgramEntity;  // Reference to a shader program entity (contains vert+frag)
		Entity pipelineEntity;       // Reference to specific pipeline
		std::vector<Entity> textureEntities;  // References to texture entities
		glm::vec4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float metallic = 0.0f;
		float roughness = 0.5f;
		bool isTransparent = false;
	};

	struct TransformComponent : IComponent {
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
		Entity parent = Entity(0);  // Optional parent for hierarchies
	};

	struct TransformMatrixComponent : IComponent {
		glm::mat4 matrix = glm::mat4(1.0f);
		glm::mat4 normalMatrix = glm::mat4(1.0f);
		bool isDirty = true;
	};

	struct UniformBufferComponent : IComponent {
		BufferWrapper buffer;
		void* mappedMemory = nullptr;
		size_t elementSize;
		size_t elementAlignment;
		size_t elementCount;
	};
}
