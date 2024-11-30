#pragma once
#include <EASTL/vector.h>

#include "Base/Memory.hpp"
#include "Common.hpp"
#include "vulkan-memory-allocator-hpp/vk_mem_alloc.hpp"


namespace spite
{
	struct ResourceAllocationCallbacks
	{
		ResourceAllocationCallbacks(const ResourceAllocationCallbacks& other) = delete;
		ResourceAllocationCallbacks(ResourceAllocationCallbacks&& other) = delete;
		ResourceAllocationCallbacks& operator=(const ResourceAllocationCallbacks& other) = delete;
		ResourceAllocationCallbacks& operator=(ResourceAllocationCallbacks&& other) = delete;

		vk::AllocationCallbacks allocationCallbacks;

		explicit ResourceAllocationCallbacks(spite::HeapAllocator& allocator);

		~ResourceAllocationCallbacks();
	};

	struct Instance
	{
		Instance(const Instance& other) = delete;
		Instance(Instance&& other) = delete;
		Instance& operator=(const Instance& other) = delete;
		Instance& operator=(Instance&& other) = delete;

		vk::Instance instance;
		std::shared_ptr<spite::ResourceAllocationCallbacks> pResourceAllocation;

		Instance(const spite::HeapAllocator& allocator,
		         std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation,
		         const eastl::vector<const char*, spite::HeapAllocator>& extensions);

		~Instance();
	};

	struct PhysicalDevice
	{
		vk::PhysicalDevice device;

		explicit PhysicalDevice(const Instance& instance);
	};

	struct Device
	{
		Device(const Device& other) = delete;
		Device(Device&& other) = delete;
		Device& operator=(const Device& other) = delete;
		Device& operator=(Device&& other) = delete;

		vk::Device device;
		std::shared_ptr<spite::ResourceAllocationCallbacks> pResourceAllocation;

		Device(const PhysicalDevice& physicalDevice,
		       const QueueFamilyIndices& indices, const spite::HeapAllocator& allocator,
		       std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~Device();
	};

	struct GpuAllocator
	{
		GpuAllocator(const GpuAllocator& other) = delete;
		GpuAllocator(GpuAllocator&& other) = delete;
		GpuAllocator& operator=(const GpuAllocator& other) = delete;
		GpuAllocator& operator=(GpuAllocator&& other) = delete;

		vma::Allocator allocator;

		GpuAllocator(const PhysicalDevice& physicalDevice, const Device& device,
		             const Instance& instance,
		             const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation);

		~GpuAllocator();
	};

	struct SwapchainDetails
	{
		SwapchainSupportDetails supportDetails;
		vk::SurfaceKHR surface;
		vk::SurfaceFormatKHR surfaceFormat;
		vk::PresentModeKHR presentMode{};
		vk::Extent2D extent;

		SwapchainDetails(const PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, const int width,
		                 const int height);
	};

	struct Swapchain
	{
		Swapchain(const Swapchain& other) = delete;
		Swapchain(Swapchain&& other) = delete;
		Swapchain& operator=(const Swapchain& other) = delete;
		Swapchain& operator=(Swapchain&& other) = delete;

		vk::SwapchainKHR swapchain;

		std::shared_ptr<spite::Device> pDevice;
		std::shared_ptr<spite::ResourceAllocationCallbacks> pResourceAllocation;


		Swapchain(const PhysicalDevice& physicalDevice, std::shared_ptr<Device> device,
		          const SwapchainDetails& details,
		          const spite::HeapAllocator& allocator,
		          std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~Swapchain();
	};

	struct SwapchainImages
	{
		SwapchainImages(const SwapchainImages& other) = delete;
		SwapchainImages(SwapchainImages&& other) = delete;
		SwapchainImages& operator=(const SwapchainImages& other) = delete;
		SwapchainImages& operator=(SwapchainImages&& other) = delete;

		std::vector<vk::Image> images{};

		SwapchainImages(const Device& device, const Swapchain& swapchain);

		~SwapchainImages();
	};

	struct ImageViews
	{
		ImageViews(const ImageViews& other) = delete;
		ImageViews(ImageViews&& other) = delete;
		ImageViews& operator=(const ImageViews& other) = delete;
		ImageViews& operator=(ImageViews&& other) = delete;

		eastl::vector<vk::ImageView, spite::HeapAllocator> imageViews{};

		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		ImageViews(std::shared_ptr<Device> device, const SwapchainImages& swapchainImages,
		           const SwapchainDetails& details,
		           std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~ImageViews();
	};

	struct RenderPass
	{
		RenderPass(const RenderPass& other) = delete;
		RenderPass(RenderPass&& other) = delete;
		RenderPass& operator=(const RenderPass& other) = delete;
		RenderPass& operator=(RenderPass&& other) = delete;

		vk::RenderPass renderPass;
		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		RenderPass(std::shared_ptr<Device> device, const SwapchainDetails& details,
		           std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~RenderPass();
	};

	struct DescriptorSetLayout
	{
		DescriptorSetLayout(const DescriptorSetLayout& other) = delete;
		DescriptorSetLayout(DescriptorSetLayout&& other) = delete;
		DescriptorSetLayout& operator=(const DescriptorSetLayout& other) = delete;
		DescriptorSetLayout& operator=(DescriptorSetLayout&& other) = delete;

		vk::DescriptorSetLayout descriptorSetLayout;

		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		DescriptorSetLayout(std::shared_ptr<Device> device,
		                    std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~DescriptorSetLayout();
	};

	struct DescriptorPool
	{
		DescriptorPool(const DescriptorPool& other) = delete;
		DescriptorPool(DescriptorPool&& other) = delete;
		DescriptorPool& operator=(const DescriptorPool& other) = delete;
		DescriptorPool& operator=(DescriptorPool&& other) = delete;

		vk::DescriptorPool descriptorPool;

		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		DescriptorPool(std::shared_ptr<Device> device, const vk::DescriptorType& type, const u32 size,
		               std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~DescriptorPool();
	};

	struct DescriptorSets
	{
		DescriptorSets(const DescriptorSets& other) = delete;
		DescriptorSets(DescriptorSets&& other) = delete;
		DescriptorSets& operator=(const DescriptorSets& other) = delete;
		DescriptorSets& operator=(DescriptorSets&& other) = delete;

		std::vector<vk::DescriptorSet> descriptorSets{};

		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		DescriptorSets(std::shared_ptr<Device> device, const DescriptorSetLayout& descriptorSetLayout,
		               const DescriptorPool& descriptorPool, const spite::HeapAllocator& allocator,
		               std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation,
		               const u32 count);

		~DescriptorSets();
	};


	struct GraphicsPipeline
	{
		GraphicsPipeline(const GraphicsPipeline& other) = delete;
		GraphicsPipeline(GraphicsPipeline&& other) = delete;
		GraphicsPipeline& operator=(const GraphicsPipeline& other) = delete;
		GraphicsPipeline& operator=(GraphicsPipeline&& other) = delete;

		vk::Pipeline graphicsPipeline;
		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		GraphicsPipeline(std::shared_ptr<Device> device, const DescriptorSetLayout& descriptorSetLayout,
		                 const SwapchainDetails& details, const RenderPass& renderPass,
		                 std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~GraphicsPipeline();
	};

	struct Framebuffers
	{
		Framebuffers(const Framebuffers& other) = delete;
		Framebuffers(Framebuffers&& other) = delete;
		Framebuffers& operator=(const Framebuffers& other) = delete;
		Framebuffers& operator=(Framebuffers&& other) = delete;

		eastl::vector<vk::Framebuffer, spite::HeapAllocator> framebuffers;

		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		Framebuffers(std::shared_ptr<Device> device, const spite::HeapAllocator& allocator,
		             const ImageViews& imageViews, const SwapchainDetails& details, const RenderPass& renderPass,
		             std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~Framebuffers();
	};

	struct CommandPool
	{
		CommandPool(const CommandPool& other) = delete;
		CommandPool(CommandPool&& other) = delete;
		CommandPool& operator=(const CommandPool& other) = delete;
		CommandPool& operator=(CommandPool&& other) = delete;

		vk::CommandPool commandPool;
		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		CommandPool(std::shared_ptr<Device> device, const u32 familyIndex,
		            const vk::CommandPoolCreateFlagBits& flagBits,
		            std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~CommandPool();
	};


	//TODO: make copy buffers func
	struct Buffer
	{
		Buffer(const Buffer& other) = delete;
		Buffer(Buffer&& other) = delete;
		Buffer& operator=(const Buffer& other) = delete;
		Buffer& operator=(Buffer&& other) = delete;

		vk::Buffer buffer;
		vma::Allocation allocation;

		std::shared_ptr<GpuAllocator> pAllocator;

		Buffer(const u64 size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags memoryProperty,
		       const vma::AllocationCreateFlags& allocationFlag, const QueueFamilyIndices& indices,
		       std::shared_ptr<GpuAllocator> allocator);

		~Buffer();
	};

	struct CommandBuffers
	{
		CommandBuffers(const CommandBuffers& other) = delete;
		CommandBuffers(CommandBuffers&& other) = delete;
		CommandBuffers& operator=(const CommandBuffers& other) = delete;
		CommandBuffers& operator=(CommandBuffers&& other) = delete;

		std::vector<vk::CommandBuffer> commandBuffers{};

		std::shared_ptr<CommandPool> pCommandPool;
		std::shared_ptr<Device> pDevice;

		CommandBuffers(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool,
		               const u32 count);

		~CommandBuffers();
	};

	//probably will need to separate fences and semaphores
	struct SyncObjects
	{
		SyncObjects(const SyncObjects& other) = delete;
		SyncObjects(SyncObjects&& other) = delete;
		SyncObjects& operator=(const SyncObjects& other) = delete;
		SyncObjects& operator=(SyncObjects&& other) = delete;

		std::vector<vk::Semaphore> imageAvailableSemaphores{};
		std::vector<vk::Semaphore> renderFinishedSemaphores{};
		std::vector<vk::Fence> inFlightFences{};

		std::shared_ptr<Device> pDevice;
		std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

		SyncObjects(std::shared_ptr<Device> device, const u32 count,
		            std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation);

		~SyncObjects();
	};
}
