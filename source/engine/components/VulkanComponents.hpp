#pragma once
#include <glm/fwd.hpp>

#include "Application/AppConifg.hpp"

#include "ecs/Core.hpp"

#include "base/VmaUsage.hpp"
#include "Engine/Common.hpp"
#include "engine/VulkanBuffer.hpp"
#include "engine/VulkanImages.hpp"

namespace spite
{
	class InputManager;
	class WindowManager;

	struct VulkanInitRequest : IEventComponent
	{
	};

	struct AllocationCallbacksComponent : ISingletonComponent
	{
		vk::AllocationCallbacks allocationCallbacks;
	};

	struct InputManagerComponent : ISingletonComponent
	{
		std::shared_ptr<InputManager> inputManager;
	};

	struct WindowManagerComponent : ISingletonComponent
	{
		std::shared_ptr<WindowManager> windowManager;
	};

	// Core Vulkan infrastructure components
	struct VulkanInstanceComponent : ISingletonComponent
	{
		vk::Instance instance;
		//std::vector<const char*> enabledExtensions{};
	};

	struct DebugMessengerComponent : ISingletonComponent
	{
		VkDebugUtilsMessengerEXT messenger{};
	};

	struct PhysicalDeviceComponent : ISingletonComponent
	{
		vk::PhysicalDevice device;
		vk::PhysicalDeviceProperties properties;
		vk::PhysicalDeviceFeatures features;
		vk::PhysicalDeviceMemoryProperties memoryProperties;
	};

	struct SurfaceComponent : ISingletonComponent
	{
		vk::SurfaceKHR surface;
		vk::PresentModeKHR presentMode{};
		vk::SurfaceFormatKHR surfaceFormat;
		vk::SurfaceCapabilitiesKHR capabilities;
	};

	struct DeviceComponent : ISingletonComponent
	{
		vk::Device device;
		QueueFamilyIndices queueFamilyIndices;
	};

	struct GpuAllocatorComponent : ISingletonComponent
	{
		vma::Allocator allocator;
	};

	struct SwapchainComponent : ISingletonComponent
	{
		vk::SwapchainKHR swapchain;
		vk::Format imageFormat{};
		//vk::ColorSpaceKHR colorSpace;
		vk::Extent2D extent;
		std::vector<vk::Image> images{};
		std::vector<vk::ImageView> imageViews{};
	};

	struct QueueComponent : ISingletonComponent
	{
		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
		vk::Queue transferQueue;

		u32 graphicsQueueIndex = 0;
		u32 presentQueueIndex = 0;
		u32 transferQueueIndex = 0;
	};

	struct RenderPassComponent : IComponent
	{
		vk::RenderPass renderPass;
	};

	struct FramebufferComponent : IComponent
	{
		std::vector<vk::Framebuffer> framebuffers{};
	};

	//struct GeometryRenderPassComponent : ISingletonComponent
	//{
	//	vk::RenderPass renderPass;
	//};

	//struct GeometryFramebufferComponent : ISingletonComponent
	//{
	//	std::vector<vk::Framebuffer> framebuffers{};
	//};

	//struct DepthRenderPassComponent : ISingletonComponent
	//{
	//	vk::RenderPass renderPass;
	//};

	//struct DepthFramebufferComponent : ISingletonComponent
	//{
	//	std::vector<vk::Framebuffer> framebuffers{};
	//};

	struct FrameDataComponent : ISingletonComponent
	{
		u32 imageIndex = 0;
		u32 currentFrame = 0;
	};

	//Resource components

	struct CommandPoolComponent : ISingletonComponent
	{
		vk::CommandPool graphicsCommandPool;
		vk::CommandPool transferCommandPool;
	};

	struct CommandBufferComponent : ISingletonComponent
	{
		std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> primaryBuffers{};
		std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> geometryBuffers{};
		std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> depthBuffers{};
		std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> lightBuffers{};
		//	u32 currentBufferIndex = 0;
	};

	struct SynchronizationComponent : ISingletonComponent
	{
		std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores{};
		std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores{};
		std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> inFlightFences{};
		u32 currentFrame = 0;
	};

	struct ShaderComponent : IComponent
	{
		vk::ShaderModule shaderModule;
		vk::ShaderStageFlagBits stage{};
		std::string entryPoint = "main";
		std::string filePath; // For hot reloading
	};

	struct DescriptorSetLayoutComponent : IComponent
	{
		vk::DescriptorSetLayout layout;
		vk::DescriptorType type{};
		//UNUSED
		u32 bindingIndex = 0;
		vk::ShaderStageFlags stages;
	};

	struct DescriptorPoolComponent : IComponent
	{
		vk::DescriptorPool pool{};
		u32 maxSets = 0;
	};

	struct DescriptorSetsComponent : IComponent
	{
		std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets{};
		u32 dynamicOffset = 0;
	};

	//struct DescriptorUboReference : IComponent {
	//	std::vector<Entity> uboEntities;
	//	std::vector<vk::DescriptorType> descriptorTypes;
	//	std::vector<u32> bindingIndices;
	//};

	struct PipelineLayoutComponent : IComponent
	{
		vk::PipelineLayout layout;
		std::vector<Entity> descriptorSetLayoutEntities{};
	};

	struct MeshComponent : IComponent
	{
		BufferWrapper vertexBuffer;
		BufferWrapper indexBuffer;
		u32 vertexCount = 0;
		u32 indexCount = 0;
		vk::DeviceSize vertexBufferOffset = 0;
		bool isDirty = false; // For dynamic meshes
	};

	struct VertexInputData
	{
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions{};
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};

		bool operator==(const VertexInputData& obj) const
		{
			if (bindingDescriptions.size() != obj.bindingDescriptions.size() ||
				attributeDescriptions.size() != obj.attributeDescriptions.size())
			{
				return false;
			}

			for (const auto& bindDescr : bindingDescriptions)
			{
				if (std::ranges::find(obj.bindingDescriptions,
				                      bindDescr) == obj.bindingDescriptions.end())
				{
					return false;
				}
			}

			for (const auto& attrDescr : attributeDescriptions)
			{
				if (std::ranges::find(obj.attributeDescriptions,
				                      attrDescr) == obj.attributeDescriptions.end())
				{
					return false;
				}
			}
			return true;
		}

		bool operator !=(const VertexInputData& obj) const
		{
			return !(*this == obj);
		}
	};

	//TODO REMOVE
	//is attached to model entity 
	struct VertexInputComponent : IComponent
	{
		VertexInputData vertexInputData;
	};

	struct PipelineComponent : IComponent
	{
		vk::Pipeline pipeline;
		Entity pipelineLayoutEntity;
		VertexInputData vertexInputData;
	};

	struct DepthPipelineTag : IComponent
	{
	};

	struct MaterialComponent : IComponent
	{
		Entity shaderProgramEntity; // Reference to a shader program entity (contains vert+frag)
		Entity pipelineEntity; // Reference to specific pipeline
		std::vector<Entity> textureEntities; // References to texture entities
		glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
		float metallic = 0.0f;
		float roughness = 0.5f;
		bool isTransparent = false;
	};

	struct UniformBuffer
	{
		BufferWrapper buffer;
		void* mappedMemory;
	};

	struct UniformBufferComponent: IComponent
	{
		vk::DescriptorType descriptorType;
		vk::ShaderStageFlags shaderStage;
		u32 bindingIndex{};

		std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> ubos;

		sizet elementSize{};
		sizet elementAlignment{};
		sizet elementCount{};
	};

	struct UniformBufferSharedComponent: ISharedComponent
	{
		vk::DescriptorType descriptorType{};
		vk::ShaderStageFlags shaderStage{};
		u32 bindingIndex{};

		std::array<UniformBuffer, MAX_FRAMES_IN_FLIGHT> ubos;

		sizet elementSize{};
		sizet elementAlignment{};
		sizet elementCount{};
	};

	// Request component for loading a model with specified shaders
	struct ModelLoadRequest : IEventComponent
	{
		//already created entity can be specified
		Entity entity = Entity::undefined();
		std::string objFilePath;
		std::string vertShaderPath;
		std::string fragShaderPath;
		std::string name; // Optional name for entity
	};

	struct TextureComponent : ISharedComponent
	{
		//descr pool + sets per texture
		Entity descriptorEntity;
		Image texture;
		vk::ImageView imageView;
		vk::Sampler sampler;
	};

	struct TextureLoadRequest : IEventComponent
	{
		std::vector<Entity> targets; 
		std::string path{};
	};

	struct DepthImageComponent : ISingletonComponent
	{
		Image image;
		vk::ImageView imageView;
	};

	struct GBufferComponent : ISingletonComponent
	{
		Image positionImage;
		Image normalsImage;
		Image albedoImage;
		//unused for now
		Image materialImage;

		vk::ImageView positionImageView;
		vk::ImageView normalImageView;
		vk::ImageView albedoImageView;
		//unused for now
		vk::ImageView materialImageView;
	};

	struct GBufferSampler : ISingletonComponent
	{
		vk::Sampler sampler;
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

	struct PipelineReference : IComponent
	{
		Entity pipelineEntity;
	};

	struct CleanupRequest : IEventComponent
	{
	};
}
