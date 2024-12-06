#pragma once
#include <EASTL/map.h>
#include <EASTL/string.h>

#include <glm/vec3.hpp>

#include "Engine/Resources.hpp"

namespace spite
{
	class WindowManager;
	//TODO: use eastl:: smart ptrs

	//immutable objects, base for others (has to be destroyed last)
	//TODO: consider callbacks as separate mutable module
	struct BaseModule
	{
		BaseModule(const BaseModule& other) = delete;
		BaseModule(BaseModule&& other) = delete;
		BaseModule& operator=(const BaseModule& other) = delete;
		BaseModule& operator=(BaseModule&& other) = delete;

		const AllocationCallbacksWrapper allocationCallbacksWrapper;
		const vk::SurfaceKHR surface;
		const PhysicalDeviceWrapper physicalDeviceWrapper;
		const QueueFamilyIndices indices;
		const InstanceExtensions extensions;
		const InstanceWrapper instanceWrapper;
		const DeviceWrapper deviceWrapper;
		const GpuAllocatorWrapper gpuAllocatorWrapper;

		const CommandPoolWrapper transferCommandPool;

		vk::Queue transferQueue;
		vk::Queue presentQueue;
		vk::Queue graphicsQueue;

		BaseModule(spite::HeapAllocator& allocator,
		           char const* const* windowExtensions, const u32 windowExtensionCount,
		           const vk::SurfaceKHR& surface);

		~BaseModule() = default;
	};

	struct SwapchainModule
	{
		SwapchainModule(const SwapchainModule& other) = delete;
		SwapchainModule(SwapchainModule&& other) = delete;
		SwapchainModule& operator=(const SwapchainModule& other) = delete;
		SwapchainModule& operator=(SwapchainModule&& other) = delete;

		const std::shared_ptr<BaseModule> baseModule;
		SwapchainDetailsWrapper swapchainDetailsWrapper;
		SwapchainWrapper swapchainWrapper;
		RenderPassWrapper renderPassWrapper;
		SwapchainImagesWrapper swapchainImagesWrapper;
		ImageViewsWrapper imageViewsWrapper;
		FramebuffersWrapper framebuffersWrapper;

		SwapchainModule(std::shared_ptr<BaseModule> baseModulePtr, const spite::HeapAllocator& allocator,
		                const int width, const int height);

		void recreate(const int width, const int height);

		~SwapchainModule() = default;
	};

	struct DescriptorModule
	{
		DescriptorModule(const DescriptorModule& other) = delete;
		DescriptorModule(DescriptorModule&& other) = delete;
		DescriptorModule& operator=(const DescriptorModule& other) = delete;
		DescriptorModule& operator=(DescriptorModule&& other) = delete;

		const std::shared_ptr<BaseModule> baseModule;
		DescriptorSetLayoutWrapper descriptorSetLayoutWrapper;
		DescriptorPoolWrapper descriptorPoolWrapper;
		DescriptorSetsWrapper descriptorSetsWrapper;

		DescriptorModule(std::shared_ptr<BaseModule> baseModulePtr, const vk::DescriptorType& type, const u32 size,
		                 const BufferWrapper& bufferWrapper,
		                 const sizet bufferElementSize,
		                 const spite::HeapAllocator& allocator);

		~DescriptorModule() = default;
	};


	//TODO: replace buffer allocator to stack/linear
	struct ShaderServiceModule
	{
		ShaderServiceModule(const ShaderServiceModule& other) = delete;
		ShaderServiceModule(ShaderServiceModule&& other) = delete;
		ShaderServiceModule& operator=(const ShaderServiceModule& other) = delete;
		ShaderServiceModule& operator=(ShaderServiceModule&& other) = delete;

		const std::shared_ptr<BaseModule> baseModule;
		eastl::map<eastl::string, ShaderModuleWrapper, spite::HeapAllocator> shaderModules;

		const spite::HeapAllocator& bufferAllocator;

		ShaderServiceModule(std::shared_ptr<BaseModule> baseModulePtr, const spite::HeapAllocator& allocator);

		ShaderModuleWrapper& getShaderModule(const char* shaderPath, const vk::ShaderStageFlagBits& bits);
		void removeShaderModule(const char* shaderPath);

		~ShaderServiceModule();
	};

	struct GraphicsCommandModule
	{
		GraphicsCommandModule(const GraphicsCommandModule& other) = delete;
		GraphicsCommandModule(GraphicsCommandModule&& other) = delete;
		GraphicsCommandModule& operator=(const GraphicsCommandModule& other) = delete;
		GraphicsCommandModule& operator=(GraphicsCommandModule&& other) = delete;

		const std::shared_ptr<BaseModule> baseModule;

		CommandPoolWrapper commandPoolWrapper;
		CommandBuffersWrapper commandBuffersWrapper;

		GraphicsCommandModule(std::shared_ptr<BaseModule> baseModulePtr, const vk::CommandPoolCreateFlagBits& flagBits,
		                      const u32 count);

		~GraphicsCommandModule() = default;
	};

	struct ModelDataModule
	{
		ModelDataModule(const ModelDataModule& other) = delete;
		ModelDataModule(ModelDataModule&& other) = delete;
		ModelDataModule& operator=(const ModelDataModule& other) = delete;
		ModelDataModule& operator=(ModelDataModule&& other) = delete;

		BufferWrapper modelBuffer;

		//offset for indices
		vk::DeviceSize vertSize;
		const std::shared_ptr<BaseModule> baseModule;

		ModelDataModule(std::shared_ptr<BaseModule> baseModulePtr,
		                const eastl::vector<glm::vec3>& vertices,
		                const eastl::vector<u32>& indices);

		~ModelDataModule() = default;
	};

	struct UboModule
	{
		UboModule(const UboModule& other) = delete;
		UboModule(UboModule&& other) = delete;
		UboModule& operator=(const UboModule& other) = delete;
		UboModule& operator=(UboModule&& other) = delete;

		BufferWrapper uboBuffer;
		sizet elementAlignment;
		void* memory{};

		const std::shared_ptr<BaseModule> baseModule;

		UboModule(std::shared_ptr<BaseModule> baseModulePtr,
		          sizet elementSize, sizet elementCount);

		~UboModule();
	};

	struct RenderModule
	{
		std::shared_ptr<BaseModule> baseModule;
		std::shared_ptr<SwapchainModule> swapchainModule;
		std::shared_ptr<DescriptorModule> descriptorModule;
		std::shared_ptr<GraphicsCommandModule> commandBuffersModule;

		eastl::vector<std::shared_ptr<ModelDataModule>> models;

		GraphicsPipelineWrapper graphicsPipelineWrapper;
		const SyncObjectsWrapper syncObjectsWrapper;

		u32 currentFrame;

		RenderModule(std::shared_ptr<BaseModule> baseModulePtr, std::shared_ptr<SwapchainModule> swapchainModulePtr,
		             std::shared_ptr<DescriptorModule> descriptorModulePtr,
		             std::shared_ptr<GraphicsCommandModule> commandBuffersModulePtr,
		             eastl::vector<std::shared_ptr<ModelDataModule>> models,
		             const spite::HeapAllocator& allocator,
		             const eastl::vector<eastl::tuple<ShaderModuleWrapper*, const char*>, spite::HeapAllocator>&
		             shaderModules,
		             const VertexInputDescriptionsWrapper& vertexInputDescriptions, spite::WindowManager* windowManager,
		             const u32 framesInFlight);

		void waitForFrame();

		void drawFrame();

	private:
		spite::WindowManager* m_windowManager;
		void recreateSwapchain(const vk::Result result);
	};
}
