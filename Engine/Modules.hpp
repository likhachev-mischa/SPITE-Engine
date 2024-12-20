#pragma once
#include <EASTL/hash_map.h>
#include <EASTL/string.h>

#include <glm/vec3.hpp>

#include "Engine/Resources.hpp"

namespace spite
{
	class WindowManager;
	//TODO: use eastl:: smart ptrs

	//immutable objects, core (has to be destroyed last)
	struct CoreModule
	{
		CoreModule(const CoreModule& other) = delete;
		CoreModule(CoreModule&& other) = delete;
		CoreModule& operator=(const CoreModule& other) = delete;
		CoreModule& operator=(CoreModule&& other) = delete;

		const std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks;

		const InstanceExtensions extensions;

		const InstanceWrapper instanceWrapper;
		const PhysicalDeviceWrapper physicalDeviceWrapper;

		const DebugMessengerWrapper debugMessengerWrapper;

		CoreModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
		           char const* const* windowExtensions, u32 extensionCount,
		           const spite::HeapAllocator& allocator);

		~CoreModule() = default;
	};

	//based
	//destroys surface on destruction
	struct BaseModule
	{
		BaseModule(const BaseModule& other) = delete;
		BaseModule(BaseModule&& other) = delete;
		BaseModule& operator=(const BaseModule& other) = delete;
		BaseModule& operator=(BaseModule&& other) = delete;

		const std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks;
		const std::shared_ptr<CoreModule> coreModule;

		const vk::SurfaceKHR surface;

		const QueueFamilyIndices indices;
		const DeviceWrapper deviceWrapper;
		const GpuAllocatorWrapper gpuAllocatorWrapper;

		const CommandPoolWrapper transferCommandPool;

		vk::Queue transferQueue;
		vk::Queue presentQueue;
		vk::Queue graphicsQueue;

		BaseModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
		           std::shared_ptr<CoreModule> coreModule, const spite::HeapAllocator& allocator,
		           const vk::SurfaceKHR& surface);

		~BaseModule();
	};

	struct SwapchainModule
	{
		SwapchainModule(const SwapchainModule& other) = delete;
		SwapchainModule(SwapchainModule&& other) = delete;
		SwapchainModule& operator=(const SwapchainModule& other) = delete;
		SwapchainModule& operator=(SwapchainModule&& other) = delete;

		const std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks;
		const std::shared_ptr<CoreModule> coreModule;
		const std::shared_ptr<BaseModule> baseModule;

		SwapchainDetailsWrapper swapchainDetailsWrapper;
		SwapchainWrapper swapchainWrapper;
		RenderPassWrapper renderPassWrapper;
		SwapchainImagesWrapper swapchainImagesWrapper;
		ImageViewsWrapper imageViewsWrapper;
		FramebuffersWrapper framebuffersWrapper;

		SwapchainModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
		                std::shared_ptr<CoreModule> coreModulePtr, std::shared_ptr<BaseModule> baseModulePtr,
		                const spite::HeapAllocator& allocator,
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

		const std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks;
		const std::shared_ptr<BaseModule> baseModule;

		DescriptorSetLayoutWrapper descriptorSetLayoutWrapper;
		DescriptorPoolWrapper descriptorPoolWrapper;
		DescriptorSetsWrapper descriptorSetsWrapper;

		DescriptorModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
		                 std::shared_ptr<BaseModule> baseModulePtr,
		                 const vk::DescriptorType& type, const u32 count,
		                 const u32 bindingIndex,
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

		const std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks;
		const std::shared_ptr<BaseModule> baseModule;

		eastl::hash_map<eastl::string, ShaderModuleWrapper, eastl::hash<eastl::string>, eastl::equal_to<eastl::string>,
		                spite::HeapAllocator> shaderModules;

		const spite::HeapAllocator& bufferAllocator;

		ShaderServiceModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
		                    std::shared_ptr<BaseModule> baseModulePtr,
		                    const spite::HeapAllocator& allocator);

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

		const std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks;
		const std::shared_ptr<BaseModule> baseModule;

		CommandPoolWrapper commandPoolWrapper;
		CommandBuffersWrapper primaryCommandBuffersWrapper;
		CommandBuffersWrapper secondaryCommandBuffersWrapper;

		GraphicsCommandModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
		                      std::shared_ptr<BaseModule> baseModulePtr, const vk::CommandPoolCreateFlagBits& flagBits,
		                      const u32 count);

		~GraphicsCommandModule() = default;
	};

	struct ModelDataModule
	{
		ModelDataModule(const ModelDataModule& other) = delete;
		ModelDataModule(ModelDataModule&& other) = delete;
		ModelDataModule& operator=(const ModelDataModule& other) = delete;
		ModelDataModule& operator=(ModelDataModule&& other) = delete;

		const std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks;
		const std::shared_ptr<BaseModule> baseModule;

		BufferWrapper modelBuffer;

		//offset for indices
		vk::DeviceSize vertSize{};
		u32 indicesCount;

		ModelDataModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacksPtr,
		                std::shared_ptr<BaseModule> baseModulePtr,
		                const eastl::vector<glm::vec3, spite::HeapAllocator>& vertices,
		                const eastl::vector<u32, spite::HeapAllocator>& indices);

		~ModelDataModule() = default;
	};

	//dynamic ubo
	struct UboModule
	{
		UboModule(const UboModule& other) = delete;
		UboModule(UboModule&& other) = delete;
		UboModule& operator=(const UboModule& other) = delete;
		UboModule& operator=(UboModule&& other) = delete;

		BufferWrapper uboBuffer;
		sizet elementAlignment;
		void* memory{};

		UboModule(const std::shared_ptr<CoreModule>& coreModulePtr, const std::shared_ptr<BaseModule>& baseModulePtr,
		          sizet elementSize, sizet elementCount);

		~UboModule();
	};

	struct RenderModule
	{
		RenderModule(const RenderModule& other) = delete;
		RenderModule(RenderModule&& other) = delete;
		RenderModule& operator=(const RenderModule& other) = delete;
		RenderModule& operator=(RenderModule&& other) = delete;

		std::shared_ptr<spite::WindowManager> windowManager;

		const std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks;
		std::shared_ptr<BaseModule> baseModule;
		std::shared_ptr<SwapchainModule> swapchainModule;

		eastl::vector<std::shared_ptr<DescriptorModule>, spite::HeapAllocator> descriptorModules;

		std::shared_ptr<GraphicsCommandModule> graphicsCommandBuffersModule;

		//prerecorded secondary command buffers
		eastl::vector<std::shared_ptr<CommandBuffersWrapper>, spite::HeapAllocator> extraCommandBuffers;


		eastl::vector<std::shared_ptr<ModelDataModule>, spite::HeapAllocator> models;

		GraphicsPipelineWrapper graphicsPipelineWrapper;
		const SyncObjectsWrapper syncObjectsWrapper;

		u32 currentFrame;
		u32 imageIndex{};

		RenderModule(std::shared_ptr<AllocationCallbacksWrapper> allocationCallbacks,
		             std::shared_ptr<BaseModule> baseModulePtr, std::shared_ptr<SwapchainModule> swapchainModulePtr,
		             eastl::vector<std::shared_ptr<DescriptorModule>, spite::HeapAllocator> descriptorModulesVec,
		             std::shared_ptr<GraphicsCommandModule> commandBuffersModulePtr,
		             eastl::vector<std::shared_ptr<ModelDataModule>, spite::HeapAllocator> models,
		             const spite::HeapAllocator& allocator,
		             const eastl::vector<eastl::tuple<ShaderModuleWrapper&, const char*>, spite::HeapAllocator>&
		             shaderModules,
		             const VertexInputDescriptionsWrapper& vertexInputDescriptions,
		             std::shared_ptr<spite::WindowManager> windowManagerPtr,
		             const u32 framesInFlight,
		             eastl::vector<std::shared_ptr<CommandBuffersWrapper>, spite::HeapAllocator> extraCommandBuffers);

		void waitForFrame();

		void drawFrame();

		~RenderModule();

	private:
		void recreateSwapchain(const vk::Result result);
	};
}
