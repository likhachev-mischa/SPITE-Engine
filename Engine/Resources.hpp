#pragma once
#include <EASTL/array.h>
#include <EASTL/vector.h>

#include "Base/Memory.hpp"
#include "Base/VmaUsage.hpp"

#include "Engine/Common.hpp"


namespace spite
{
	//TODO: consider eastl::shared_ptr instead of std::
	struct AllocationCallbacksWrapper
	{
		AllocationCallbacksWrapper(const AllocationCallbacksWrapper& other) = delete;
		AllocationCallbacksWrapper(AllocationCallbacksWrapper&& other) = delete;
		AllocationCallbacksWrapper& operator=(const AllocationCallbacksWrapper& other) = delete;
		AllocationCallbacksWrapper& operator=(AllocationCallbacksWrapper&& other) = delete;

		vk::AllocationCallbacks allocationCallbacks;

		explicit AllocationCallbacksWrapper(spite::HeapAllocator& allocator);

		~AllocationCallbacksWrapper();
	};

	struct InstanceExtensions
	{
		InstanceExtensions(const InstanceExtensions& other) = delete;
		InstanceExtensions& operator=(const InstanceExtensions& other) = delete;
		InstanceExtensions& operator=(InstanceExtensions&& other) = delete;

		eastl::vector<const char*, spite::HeapAllocator> extensions;

		InstanceExtensions() = default;

		InstanceExtensions(char const* const* windowExtensions, const u32 windowExtensionsCount,
		                   const spite::HeapAllocator& allocator);

		InstanceExtensions(InstanceExtensions&& other) noexcept;

		~InstanceExtensions() = default;
	};

	struct InstanceWrapper
	{
		InstanceWrapper(const InstanceWrapper& other) = delete;
		InstanceWrapper(InstanceWrapper&& other) = delete;
		InstanceWrapper& operator=(const InstanceWrapper& other) = delete;
		InstanceWrapper& operator=(InstanceWrapper&& other) = delete;

		vk::Instance instance;
		const vk::AllocationCallbacks* allocationCallbacks;

		InstanceWrapper(const spite::HeapAllocator& allocator,
		                const AllocationCallbacksWrapper& allocationCallbacksWrapper,
		                const InstanceExtensions& extensions);

		~InstanceWrapper();
	};

	struct DebugMessengerWrapper
	{
		DebugMessengerWrapper(const DebugMessengerWrapper& other) = delete;
		DebugMessengerWrapper(DebugMessengerWrapper&& other) = delete;
		DebugMessengerWrapper& operator=(const DebugMessengerWrapper& other) = delete;
		DebugMessengerWrapper& operator=(DebugMessengerWrapper&& other) = delete;

		VkDebugUtilsMessengerEXT debugMessenger;

		const vk::Instance instance;
		const vk::AllocationCallbacks* allocationCallbacks;

		//TODO: ALLOCATION CALLBACKS UNUSED (SHOULD BE, BUT NOT DONE YET)
		DebugMessengerWrapper(const InstanceWrapper& instanceWrapper,
		                      const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		~DebugMessengerWrapper();
	};

	struct PhysicalDeviceWrapper
	{
		PhysicalDeviceWrapper(const PhysicalDeviceWrapper& other) = delete;
		PhysicalDeviceWrapper(PhysicalDeviceWrapper&& other) = delete;
		PhysicalDeviceWrapper& operator=(const PhysicalDeviceWrapper& other) = delete;
		PhysicalDeviceWrapper& operator=(PhysicalDeviceWrapper&& other) = delete;

		vk::PhysicalDevice device;

		explicit PhysicalDeviceWrapper(const InstanceWrapper& instanceWrapper);

		~PhysicalDeviceWrapper() = default;
	};

	struct DeviceWrapper
	{
		DeviceWrapper(const DeviceWrapper& other) = delete;
		DeviceWrapper(DeviceWrapper&& other) = delete;
		DeviceWrapper& operator=(const DeviceWrapper& other) = delete;
		DeviceWrapper& operator=(DeviceWrapper&& other) = delete;

		vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		DeviceWrapper(const PhysicalDeviceWrapper& physicalDeviceWrapper,
		              const QueueFamilyIndices& indices, const spite::HeapAllocator& allocator,
		              const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		~DeviceWrapper();
	};

	struct GpuAllocatorWrapper
	{
		GpuAllocatorWrapper(const GpuAllocatorWrapper& other) = delete;
		GpuAllocatorWrapper(GpuAllocatorWrapper&& other) = delete;
		GpuAllocatorWrapper& operator=(const GpuAllocatorWrapper& other) = delete;
		GpuAllocatorWrapper& operator=(GpuAllocatorWrapper&& other) = delete;

		vma::Allocator allocator;

		GpuAllocatorWrapper(const PhysicalDeviceWrapper& physicalDevice, const DeviceWrapper& deviceWrapper,
		                    const InstanceWrapper& instanceWrapper,
		                    const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		~GpuAllocatorWrapper();
	};

	struct BufferWrapper
	{
		BufferWrapper(const BufferWrapper& other) = delete;
		BufferWrapper& operator=(const BufferWrapper& other) = delete;

		vk::Buffer buffer;
		vma::Allocation allocation;

		vma::Allocator allocator;
		vk::DeviceSize size{};

		BufferWrapper() = default;

		BufferWrapper(const u64 size, const vk::BufferUsageFlags& usage,
		              const vk::MemoryPropertyFlags memoryProperty,
		              const vma::AllocationCreateFlags& allocationFlag, const QueueFamilyIndices& indices,
		              const GpuAllocatorWrapper& allocatorWrapper);

		BufferWrapper(BufferWrapper&& other) noexcept;
		BufferWrapper& operator=(BufferWrapper&& other) noexcept;

		//copies only same sized buffers
		void copyBuffer(const BufferWrapper& other, const vk::Device& device, const vk::CommandPool&
		                transferCommandPool, const vk::Queue transferQueue,
		                const vk::AllocationCallbacks* allocationCallbacks) const;

		void copyMemory(const void* data, const vk::DeviceSize& memorySize, const vk::DeviceSize& localOffset) const;

		[[nodiscard]] void* mapMemory() const;
		void unmapMemory() const;

		~BufferWrapper();
	};


	struct SwapchainDetailsWrapper
	{
		SwapchainSupportDetails supportDetails;
		vk::SurfaceFormatKHR surfaceFormat;
		vk::PresentModeKHR presentMode{};
		vk::Extent2D extent;

		SwapchainDetailsWrapper(const PhysicalDeviceWrapper& physicalDeviceWrapper,
		                        const vk::SurfaceKHR& surface,
		                        const int width,
		                        const int height);

		SwapchainDetailsWrapper(const SwapchainDetailsWrapper& other);
		SwapchainDetailsWrapper& operator=(const SwapchainDetailsWrapper& other);

		SwapchainDetailsWrapper(SwapchainDetailsWrapper&& other) noexcept;
		SwapchainDetailsWrapper& operator=(SwapchainDetailsWrapper&& other) noexcept;

		~SwapchainDetailsWrapper() = default;
	};

	struct SwapchainWrapper
	{
		SwapchainWrapper(SwapchainWrapper&& other) = delete;
		SwapchainWrapper(SwapchainWrapper& other) = delete;
		SwapchainWrapper& operator=(SwapchainWrapper&& other) = delete;
		SwapchainWrapper& operator=(const SwapchainWrapper& other) = delete;

		vk::SwapchainKHR swapchain;

		const QueueFamilyIndices indices;
		const vk::SurfaceKHR surface;

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		SwapchainWrapper(const DeviceWrapper& deviceWrapper, const QueueFamilyIndices& indices,
		                 const SwapchainDetailsWrapper& swapchainDetailsWrapper,
		                 const vk::SurfaceKHR& surface,
		                 const AllocationCallbacksWrapper& allocationCallbacksWrapper);
		//should call device.waitIdle() first
		void recreate(const SwapchainDetailsWrapper& swapchainDetailsWrapper);

		~SwapchainWrapper();
	};

	struct SwapchainImagesWrapper
	{
		std::vector<vk::Image> images{};

		SwapchainImagesWrapper(const DeviceWrapper& deviceWrapper, const SwapchainWrapper& swapchainWrapper);

		SwapchainImagesWrapper(const SwapchainImagesWrapper& other);
		SwapchainImagesWrapper& operator=(const SwapchainImagesWrapper& other);

		SwapchainImagesWrapper(SwapchainImagesWrapper&& other) noexcept;
		SwapchainImagesWrapper& operator=(SwapchainImagesWrapper&& other) noexcept;

		~SwapchainImagesWrapper() = default;
	};

	struct ImageViewsWrapper
	{
		ImageViewsWrapper(const ImageViewsWrapper& other) = delete;
		ImageViewsWrapper(ImageViewsWrapper&& other) = delete;
		ImageViewsWrapper& operator=(const ImageViewsWrapper& other) = delete;
		ImageViewsWrapper& operator=(ImageViewsWrapper&& other) = delete;

		eastl::vector<vk::ImageView, spite::HeapAllocator> imageViews;

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		ImageViewsWrapper(const DeviceWrapper& deviceWrapper, const SwapchainImagesWrapper& swapchainImagesWrapper,
		                  const SwapchainDetailsWrapper& detailsWrapper,
		                  const spite::HeapAllocator& allocator,
		                  const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		void recreate(const SwapchainImagesWrapper& swapchainImagesWrapper,
		              const SwapchainDetailsWrapper& detailsWrapper);

		~ImageViewsWrapper();
	};

	struct RenderPassWrapper
	{
		RenderPassWrapper(const RenderPassWrapper& other) = delete;
		RenderPassWrapper(RenderPassWrapper&& other) = delete;
		RenderPassWrapper& operator=(const RenderPassWrapper& other) = delete;
		RenderPassWrapper& operator=(RenderPassWrapper&& other) = delete;

		vk::RenderPass renderPass;
		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		RenderPassWrapper(const DeviceWrapper& deviceWrapper, const SwapchainDetailsWrapper& detailsWrapper,
		                  const AllocationCallbacksWrapper& allocationCallbacksWrapper);
		void recreate(const SwapchainDetailsWrapper& detailsWrapper);

		~RenderPassWrapper();
	};

	struct DescriptorSetLayoutWrapper
	{
		DescriptorSetLayoutWrapper(const DescriptorSetLayoutWrapper& other) = delete;
		DescriptorSetLayoutWrapper(DescriptorSetLayoutWrapper&& other) = delete;
		DescriptorSetLayoutWrapper& operator=(const DescriptorSetLayoutWrapper& other) = delete;
		DescriptorSetLayoutWrapper& operator=(DescriptorSetLayoutWrapper&& other) = delete;

		vk::DescriptorSetLayout descriptorSetLayout;

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		DescriptorSetLayoutWrapper(const DeviceWrapper& deviceWrapper, const vk::DescriptorType& type,
		                           const u32 bindingIndex,
		                           const vk::ShaderStageFlags& stage,
		                           const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		~DescriptorSetLayoutWrapper();
	};

	struct DescriptorPoolWrapper
	{
		DescriptorPoolWrapper(const DescriptorPoolWrapper& other) = delete;
		DescriptorPoolWrapper(DescriptorPoolWrapper&& other) = delete;
		DescriptorPoolWrapper& operator=(const DescriptorPoolWrapper& other) = delete;
		DescriptorPoolWrapper& operator=(DescriptorPoolWrapper&& other) = delete;

		vk::DescriptorPool descriptorPool;

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		DescriptorPoolWrapper(const DeviceWrapper& deviceWrapper, const vk::DescriptorType& type, const u32 size,
		                      const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		~DescriptorPoolWrapper();
	};

	//dynamic sets
	struct DescriptorSetsWrapper
	{
		DescriptorSetsWrapper(const DescriptorSetsWrapper& other) = delete;
		DescriptorSetsWrapper(DescriptorSetsWrapper&& other) = delete;
		DescriptorSetsWrapper& operator=(const DescriptorSetsWrapper& other) = delete;
		DescriptorSetsWrapper& operator=(DescriptorSetsWrapper&& other) = delete;

		std::vector<vk::DescriptorSet> descriptorSets{};
		const u32 dynamicOffset;

		const vk::DescriptorPool descriptorPool;
		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		DescriptorSetsWrapper(const DeviceWrapper& deviceWrapper,
		                      const DescriptorSetLayoutWrapper& descriptorSetLayoutWrapper,
		                      const DescriptorPoolWrapper& descriptorPoolWrapper,
		                      const spite::HeapAllocator& allocator,
		                      const AllocationCallbacksWrapper& allocationCallbacksWrapper,
		                      const u32 count,
		                      const u32 bindingIndex,
		                      const BufferWrapper& bufferWrapper,
		                      const sizet bufferElementSize);

		~DescriptorSetsWrapper();
	};

	struct ShaderModuleWrapper
	{
		ShaderModuleWrapper& operator=(const ShaderModuleWrapper& other) = delete;

		vk::ShaderModule shaderModule;
		vk::ShaderStageFlagBits stage;

		vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		ShaderModuleWrapper(const DeviceWrapper& deviceWrapper, const eastl::vector<char, spite::HeapAllocator>& code,
		                    const vk::ShaderStageFlagBits& stageFlag,
		                    const AllocationCallbacksWrapper& allocationCallbacksWrapper);
		ShaderModuleWrapper(const ShaderModuleWrapper& other) = delete;

		ShaderModuleWrapper& operator=(ShaderModuleWrapper&& other) noexcept;

		ShaderModuleWrapper(ShaderModuleWrapper&& other) noexcept;

		~ShaderModuleWrapper();
	};

	struct VertexInputDescriptionsWrapper
	{
		eastl::vector<vk::VertexInputBindingDescription, spite::HeapAllocator> bindingDescriptions;
		eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator> attributeDescriptions;

		VertexInputDescriptionsWrapper(
			const eastl::vector<vk::VertexInputBindingDescription, spite::HeapAllocator>& bindingDescriptions,
			const eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator>
			& attributeDescriptions);
	};

	struct GraphicsPipelineWrapper
	{
		GraphicsPipelineWrapper(const GraphicsPipelineWrapper& other) = delete;
		GraphicsPipelineWrapper& operator=(const GraphicsPipelineWrapper& other) = delete;

		vk::PipelineLayout pipelineLayout;
		vk::Pipeline graphicsPipeline;
		eastl::vector<vk::PipelineShaderStageCreateInfo, spite::HeapAllocator> shaderStages;

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

		vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		GraphicsPipelineWrapper() = default;
		GraphicsPipelineWrapper(const DeviceWrapper& deviceWrapper,
		                        const eastl::vector<DescriptorSetLayoutWrapper*, spite::HeapAllocator>&
		                        descriptorSetLayouts,
		                        const SwapchainDetailsWrapper& detailsWrapper,
		                        const RenderPassWrapper& renderPassWrapper, const spite::HeapAllocator& allocator,
		                        const eastl::vector<
			                        eastl::tuple<ShaderModuleWrapper&, const char*>, spite::HeapAllocator>
		                        & shaderModules, const VertexInputDescriptionsWrapper& vertexInputDescription,
		                        const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		GraphicsPipelineWrapper(GraphicsPipelineWrapper&& other) noexcept;

		GraphicsPipelineWrapper& operator=(GraphicsPipelineWrapper&& other) noexcept;

		void recreate(const SwapchainDetailsWrapper& detailsWrapper,
		              const RenderPassWrapper& renderPassWrapper);

		~GraphicsPipelineWrapper();
	};

	struct FramebuffersWrapper
	{
		FramebuffersWrapper(const FramebuffersWrapper& other) = delete;
		FramebuffersWrapper(FramebuffersWrapper&& other) = delete;
		FramebuffersWrapper& operator=(const FramebuffersWrapper& other) = delete;
		FramebuffersWrapper& operator=(FramebuffersWrapper&& other) = delete;

		eastl::vector<vk::Framebuffer, spite::HeapAllocator> framebuffers;

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		FramebuffersWrapper(const DeviceWrapper& deviceWrapper, const spite::HeapAllocator& allocator,
		                    const ImageViewsWrapper& imageViewsWrapper, const SwapchainDetailsWrapper& detailsWrapper,
		                    const RenderPassWrapper& renderPassWrapper,
		                    const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		void recreate(const SwapchainDetailsWrapper& detailsWrapper,
		              const ImageViewsWrapper& imageViewsWrapper,
		              const RenderPassWrapper& renderPassWrapper);

		~FramebuffersWrapper();
	};

	struct CommandPoolWrapper
	{
		CommandPoolWrapper(const CommandPoolWrapper& other) = delete;
		CommandPoolWrapper(CommandPoolWrapper&& other) = delete;
		CommandPoolWrapper& operator=(const CommandPoolWrapper& other) = delete;
		CommandPoolWrapper& operator=(CommandPoolWrapper&& other) = delete;

		vk::CommandPool commandPool;

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		CommandPoolWrapper(const DeviceWrapper& deviceWrapper, const u32 familyIndex,
		                   const vk::CommandPoolCreateFlagBits& flagBits,
		                   const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		~CommandPoolWrapper();
	};


	struct CommandBuffersWrapper
	{
		CommandBuffersWrapper(const CommandBuffersWrapper& other) = delete;
		CommandBuffersWrapper& operator=(const CommandBuffersWrapper& other) = delete;

		std::vector<vk::CommandBuffer> commandBuffers{};

		vk::CommandPool commandPool;
		vk::Device device;

		CommandBuffersWrapper(const DeviceWrapper& deviceWrapper, const CommandPoolWrapper& commandPoolWrapper,
		                      const vk::CommandBufferLevel& level,
		                      const u32 count);

		CommandBuffersWrapper(CommandBuffersWrapper&& other) noexcept;

		CommandBuffersWrapper& operator=(CommandBuffersWrapper&& other) noexcept;

		~CommandBuffersWrapper();
	};

	//probably will need to separate fences and semaphores
	struct SyncObjectsWrapper
	{
		SyncObjectsWrapper(const SyncObjectsWrapper& other) = delete;
		SyncObjectsWrapper(SyncObjectsWrapper&& other) = delete;
		SyncObjectsWrapper& operator=(const SyncObjectsWrapper& other) = delete;
		SyncObjectsWrapper& operator=(SyncObjectsWrapper&& other) = delete;

		std::vector<vk::Semaphore> imageAvailableSemaphores{};
		std::vector<vk::Semaphore> renderFinishedSemaphores{};
		std::vector<vk::Fence> inFlightFences{};

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		SyncObjectsWrapper(const DeviceWrapper& deviceWrapper, const u32 count,
		                   const AllocationCallbacksWrapper& allocationCallbacksWrapper);

		~SyncObjectsWrapper();
	};
}
