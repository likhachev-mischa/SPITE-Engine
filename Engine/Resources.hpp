#pragma once
#include <EASTL/array.h>
#include <EASTL/vector.h>

#include "Base/Memory.hpp"
#include "Common.hpp"
#include "vulkan-memory-allocator-hpp/vk_mem_alloc.hpp"


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
		InstanceExtensions(InstanceExtensions&& other) = delete;
		InstanceExtensions& operator=(const InstanceExtensions& other) = delete;
		InstanceExtensions& operator=(InstanceExtensions&& other) = delete;

		eastl::vector<const char*, spite::HeapAllocator> extensions;

		InstanceExtensions(char const* const* windowExtensions, const u32 windowExtensionsCount,
		                   const spite::HeapAllocator& allocator);

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
		                const eastl::vector<const char*, spite::HeapAllocator>& extensions);

		~InstanceWrapper();
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
		                    const vk::AllocationCallbacks& allocationCallbacksWrapper);

		~GpuAllocatorWrapper();
	};

	struct SwapchainDetailsWrapper
	{
		SwapchainSupportDetails supportDetails;
		vk::SurfaceKHR surface;
		vk::SurfaceFormatKHR surfaceFormat;
		vk::PresentModeKHR presentMode{};
		vk::Extent2D extent;

		SwapchainDetailsWrapper(const PhysicalDeviceWrapper& physicalDeviceWrapper, const vk::SurfaceKHR& surface,
		                        const int width,
		                        const int height);
	};

	struct SwapchainWrapper
	{
		SwapchainWrapper(const SwapchainWrapper& other) = delete;
		SwapchainWrapper(SwapchainWrapper&& other) = delete;
		SwapchainWrapper& operator=(const SwapchainWrapper& other) = delete;
		SwapchainWrapper& operator=(SwapchainWrapper&& other) = delete;

		vk::SwapchainKHR swapchain;

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		SwapchainWrapper(const PhysicalDeviceWrapper& physicalDeviceWrapper, const DeviceWrapper& deviceWrapper,
		                 const SwapchainDetailsWrapper& swapchainDetailsWrapper,
		                 const AllocationCallbacksWrapper& allocationCallbacksWrapper,
		                 const spite::HeapAllocator& allocator);

		~SwapchainWrapper();
	};

	struct SwapchainImages
	{
		SwapchainImages(const SwapchainImages& other) = delete;
		SwapchainImages(SwapchainImages&& other) = delete;
		SwapchainImages& operator=(const SwapchainImages& other) = delete;
		SwapchainImages& operator=(SwapchainImages&& other) = delete;

		std::vector<vk::Image> images{};

		SwapchainImages(const DeviceWrapper& deviceWrapper, const SwapchainWrapper& swapchainWrapper);

		~SwapchainImages();
	};

	struct ImageViewsWrapper
	{
		ImageViewsWrapper(const ImageViewsWrapper& other) = delete;
		ImageViewsWrapper(ImageViewsWrapper&& other) = delete;
		ImageViewsWrapper& operator=(const ImageViewsWrapper& other) = delete;
		ImageViewsWrapper& operator=(ImageViewsWrapper&& other) = delete;

		eastl::vector<vk::ImageView, spite::HeapAllocator> imageViews{};

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		ImageViewsWrapper(const DeviceWrapper& deviceWrapper, const SwapchainImages& swapchainImagesWrapper,
		                  const SwapchainDetailsWrapper& detailsWrapper,
		                  const AllocationCallbacksWrapper& resourceAllocationWrapper);

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

		DescriptorSetLayoutWrapper(const DeviceWrapper& deviceWrapper,
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

	struct DescriptorSetsWrapper
	{
		DescriptorSetsWrapper(const DescriptorSetsWrapper& other) = delete;
		DescriptorSetsWrapper(DescriptorSetsWrapper&& other) = delete;
		DescriptorSetsWrapper& operator=(const DescriptorSetsWrapper& other) = delete;
		DescriptorSetsWrapper& operator=(DescriptorSetsWrapper&& other) = delete;

		std::vector<vk::DescriptorSet> descriptorSets{};

		const vk::DescriptorPool descriptorPool;
		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		DescriptorSetsWrapper(const DeviceWrapper& deviceWrapper,
		                      const DescriptorSetLayoutWrapper& descriptorSetLayoutWrapper,
		                      const DescriptorPoolWrapper& descriptorPoolWrapper,
		                      const spite::HeapAllocator& allocator,
		                      const AllocationCallbacksWrapper& allocationCallbacksWrapper,
		                      const u32 count);

		~DescriptorSetsWrapper();
	};

	struct ShaderModuleWrapper
	{
		ShaderModuleWrapper(const ShaderModuleWrapper& other) = delete;
		ShaderModuleWrapper(ShaderModuleWrapper&& other) = delete;
		ShaderModuleWrapper& operator=(const ShaderModuleWrapper& other) = delete;
		ShaderModuleWrapper& operator=(ShaderModuleWrapper&& other) = delete;

		vk::ShaderModule shaderModule;
		const vk::ShaderStageFlagBits stage;

		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		ShaderModuleWrapper(const DeviceWrapper& deviceWrapper, const std::vector<char>& code,
		                    const vk::ShaderStageFlagBits& stageFlag,
		                    const AllocationCallbacksWrapper& allocationCallbacksWrapper);
		~ShaderModuleWrapper();
	};

	struct VertexInputDescriptions
	{
		eastl::vector<vk::VertexInputBindingDescription, spite::HeapAllocator> bindingDescriptions;
		eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator> attributeDescriptions;

		VertexInputDescriptions(
			eastl::vector<vk::VertexInputBindingDescription, spite::HeapAllocator> bindingDescriptions,
			eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator>
			attributeDescriptions);
	};

	struct GraphicsPipelineWrapper
	{
		GraphicsPipelineWrapper(const GraphicsPipelineWrapper& other) = delete;
		GraphicsPipelineWrapper(GraphicsPipelineWrapper&& other) = delete;
		GraphicsPipelineWrapper& operator=(const GraphicsPipelineWrapper& other) = delete;
		GraphicsPipelineWrapper& operator=(GraphicsPipelineWrapper&& other) = delete;

		vk::Pipeline graphicsPipeline;
		const vk::Device device;
		const vk::AllocationCallbacks* allocationCallbacks;

		GraphicsPipelineWrapper(const DeviceWrapper& deviceWrapper,
		                        const DescriptorSetLayoutWrapper& descriptorSetLayoutWrapper,
		                        const SwapchainDetailsWrapper& detailsWrapper,
		                        const RenderPassWrapper& renderPassWrapper, const spite::HeapAllocator& allocator,
		                        eastl::array<std::tuple<std::shared_ptr<ShaderModuleWrapper>, const char*>>&
		                        shaderModules, const VertexInputDescriptions& vertexInputDescription,
		                        const AllocationCallbacksWrapper& allocationCallbacksWrapper);

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


	//TODO: make copy buffers func
	struct BufferWrapper
	{
		BufferWrapper(const BufferWrapper& other) = delete;
		BufferWrapper(BufferWrapper&& other) = delete;
		BufferWrapper& operator=(const BufferWrapper& other) = delete;
		BufferWrapper& operator=(BufferWrapper&& other) = delete;

		vk::Buffer buffer;
		vma::Allocation allocation;

		const vma::Allocator allocator;

		BufferWrapper(const u64 size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags memoryProperty,
		              const vma::AllocationCreateFlags& allocationFlag, const QueueFamilyIndices& indices,
		              const GpuAllocatorWrapper& allocatorWrapper);

		~BufferWrapper();
	};

	struct CommandBuffersWrapper
	{
		CommandBuffersWrapper(const CommandBuffersWrapper& other) = delete;
		CommandBuffersWrapper(CommandBuffersWrapper&& other) = delete;
		CommandBuffersWrapper& operator=(const CommandBuffersWrapper& other) = delete;
		CommandBuffersWrapper& operator=(CommandBuffersWrapper&& other) = delete;

		std::vector<vk::CommandBuffer> commandBuffers{};

		const vk::CommandPool commandPool;
		const vk::Device device;

		CommandBuffersWrapper(const DeviceWrapper& deviceWrapper, const CommandPoolWrapper& commandPoolWrapper,
		                      const u32 count);

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
