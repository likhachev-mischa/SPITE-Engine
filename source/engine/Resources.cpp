#include "Resources.hpp"

#include <EASTL/tuple.h>

#include "Base/Assert.hpp"

#include "Engine/Debug.hpp"
#include "Engine/ResourcesCore.hpp"
#include "Engine/VulkanAllocator.hpp"

namespace spite
{
	AllocationCallbacksWrapper::AllocationCallbacksWrapper(spite::HeapAllocator& allocator)
	{
		allocationCallbacks = vk::AllocationCallbacks(
			&allocator,
			&vkAllocate,
			&vkReallocate,
			&vkFree,
			//		&vkAllocationCallback,
			//		&vkFreeCallback);
			nullptr, nullptr);
	}

	AllocationCallbacksWrapper::~AllocationCallbacksWrapper()
	{
		allocationCallbacks.pUserData = nullptr;
	}

	InstanceExtensions::InstanceExtensions(char const* const* windowExtensions, const u32 windowExtensionsCount,
	                                       const spite::HeapAllocator& allocator) : extensions(allocator)
	{
		extensions = getRequiredExtensions(allocator, windowExtensions, windowExtensionsCount);
	}

	InstanceExtensions::InstanceExtensions(InstanceExtensions&& other) noexcept: extensions(std::move(other.extensions))
	{
	}

	InstanceWrapper::InstanceWrapper(const spite::HeapAllocator& allocator,
	                                 const AllocationCallbacksWrapper& allocationCallbacksWrapper,
	                                 const InstanceExtensions& extensions):
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		instance = spite::createInstance(allocator, *allocationCallbacks, extensions.extensions);
	}

	InstanceWrapper::~InstanceWrapper()
	{
		instance.destroy(allocationCallbacks);
	}


	DebugMessengerWrapper::DebugMessengerWrapper(const InstanceWrapper& instanceWrapper,
	                                             const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		instance(instanceWrapper.instance), allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		if constexpr (!ENABLE_VALIDATION_LAYERS)
		{
			return;
		}
		debugMessenger = createDebugUtilsMessenger(instance, nullptr);
	}

	DebugMessengerWrapper::~DebugMessengerWrapper()
	{
		if constexpr (!ENABLE_VALIDATION_LAYERS)
		{
			return;
		}
		destroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
	}


	PhysicalDeviceWrapper::PhysicalDeviceWrapper(const InstanceWrapper& instanceWrapper)
	{
		device = spite::getPhysicalDevice(instanceWrapper.instance);
	}

	DeviceWrapper::DeviceWrapper(const PhysicalDeviceWrapper& physicalDeviceWrapper, const QueueFamilyIndices& indices,
	                             const spite::HeapAllocator& allocator,
	                             const AllocationCallbacksWrapper& allocationCallbacksWrapper): allocationCallbacks(&
		allocationCallbacksWrapper.allocationCallbacks)
	{
		device = spite::createDevice(indices, physicalDeviceWrapper.device, allocator, allocationCallbacks);
	}

	DeviceWrapper::~DeviceWrapper()
	{
		device.destroy(allocationCallbacks);
	}

	GpuAllocatorWrapper::GpuAllocatorWrapper(const PhysicalDeviceWrapper& physicalDevice,
	                                         const DeviceWrapper& deviceWrapper,
	                                         const InstanceWrapper& instanceWrapper,
	                                         const AllocationCallbacksWrapper& allocationCallbacksWrapper)
	{
		allocator = spite::createVmAllocator(physicalDevice.device, deviceWrapper.device, instanceWrapper.instance,
		                                     &allocationCallbacksWrapper.allocationCallbacks);
	}

	GpuAllocatorWrapper::~GpuAllocatorWrapper()
	{
		allocator.destroy();
	}

	SwapchainDetailsWrapper& SwapchainDetailsWrapper::operator=(const SwapchainDetailsWrapper& other)
	{
		if (this == &other)
		{
			return *this;
		}

		supportDetails = other.supportDetails;
		surfaceFormat = other.surfaceFormat;
		presentMode = other.presentMode;
		extent = other.extent;

		return *this;
	}

	SwapchainDetailsWrapper::SwapchainDetailsWrapper(SwapchainDetailsWrapper&& other) noexcept: supportDetails(
			std::move(other.supportDetails)),
		surfaceFormat(other.surfaceFormat),
		presentMode(other.presentMode),
		extent(other.extent)
	{
	}

	SwapchainDetailsWrapper& SwapchainDetailsWrapper::operator=(SwapchainDetailsWrapper&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		supportDetails = std::move(other.supportDetails);
		surfaceFormat = other.surfaceFormat;
		presentMode = other.presentMode;
		extent = other.extent;
		return *this;
	}

	SwapchainDetailsWrapper::SwapchainDetailsWrapper(const SwapchainDetailsWrapper& other):
		supportDetails(other.supportDetails),
		surfaceFormat(other.surfaceFormat),
		presentMode(other.presentMode),
		extent(other.extent)
	{
	}

	SwapchainDetailsWrapper::SwapchainDetailsWrapper(const PhysicalDeviceWrapper& physicalDeviceWrapper,
	                                                 const vk::SurfaceKHR& surface,
	                                                 const int width, const int height)
	{
		supportDetails = querySwapchainSupport(physicalDeviceWrapper.device, surface);
		surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
		presentMode = chooseSwapPresentMode(supportDetails.presentModes);
		extent = chooseSwapExtent(supportDetails.capabilities, width, height);
	}

	SwapchainWrapper::SwapchainWrapper(const DeviceWrapper& deviceWrapper,
	                                   const QueueFamilyIndices& indices,
	                                   const SwapchainDetailsWrapper& swapchainDetailsWrapper,
	                                   const vk::SurfaceKHR& surface,
	                                   const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		indices(indices), surface(surface),
		device(deviceWrapper.device), allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)

	{
		swapchain = spite::createSwapchain(device, indices, surface,
		                                   swapchainDetailsWrapper.supportDetails,
		                                   swapchainDetailsWrapper.extent,
		                                   swapchainDetailsWrapper.surfaceFormat, swapchainDetailsWrapper.presentMode,
		                                   allocationCallbacks);
	}

	void SwapchainWrapper::recreate(const SwapchainDetailsWrapper& swapchainDetailsWrapper)
	{
		device.destroySwapchainKHR(swapchain, allocationCallbacks);

		swapchain = spite::createSwapchain(device, indices, surface,
		                                   swapchainDetailsWrapper.supportDetails,
		                                   swapchainDetailsWrapper.extent,
		                                   swapchainDetailsWrapper.surfaceFormat, swapchainDetailsWrapper.presentMode,
		                                   allocationCallbacks);
	}

	SwapchainWrapper::~SwapchainWrapper()
	{
		device.destroySwapchainKHR(swapchain, allocationCallbacks);
	}

	SwapchainImagesWrapper::SwapchainImagesWrapper(const DeviceWrapper& deviceWrapper,
	                                               const SwapchainWrapper& swapchainWrapper)
	{
		images = getSwapchainImages(deviceWrapper.device, swapchainWrapper.swapchain);
	}

	SwapchainImagesWrapper::SwapchainImagesWrapper(const SwapchainImagesWrapper& other): images(other.images)
	{
	}

	SwapchainImagesWrapper& SwapchainImagesWrapper::operator=(const SwapchainImagesWrapper& other)
	{
		if (this == &other)
		{
			return *this;
		}

		images.clear();
		images = other.images;
		return *this;
	}

	SwapchainImagesWrapper::SwapchainImagesWrapper(SwapchainImagesWrapper&& other) noexcept: images(
		std::move(other.images))
	{
	}

	SwapchainImagesWrapper& SwapchainImagesWrapper::operator=(SwapchainImagesWrapper&& other) noexcept
	{
		images.clear();
		images = std::move(other.images);
		return *this;
	}

	ImageViewsWrapper::ImageViewsWrapper(const DeviceWrapper& deviceWrapper,
	                                     const SwapchainImagesWrapper& swapchainImagesWrapper,
	                                     const SwapchainDetailsWrapper& detailsWrapper,
	                                     const spite::HeapAllocator& allocator,
	                                     const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		imageViews(allocator),
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		imageViews = createImageViews(device, swapchainImagesWrapper.images, detailsWrapper.surfaceFormat.format,
		                              allocator, allocationCallbacks);
	}

	void ImageViewsWrapper::recreate(const SwapchainImagesWrapper& swapchainImagesWrapper,
	                                 const SwapchainDetailsWrapper& detailsWrapper)
	{
		for (const auto& imageView : imageViews)
		{
			device.destroyImageView(imageView, allocationCallbacks);
		}
		imageViews.clear();

		imageViews = createImageViews(device, swapchainImagesWrapper.images, detailsWrapper.surfaceFormat.format,
		                              imageViews.get_allocator(), allocationCallbacks);
	}

	ImageViewsWrapper::~ImageViewsWrapper()
	{
		for (const auto& imageView : imageViews)
		{
			device.destroyImageView(imageView, allocationCallbacks);
		}
		imageViews.clear();
	}

	RenderPassWrapper::RenderPassWrapper(const DeviceWrapper& deviceWrapper,
	                                     const SwapchainDetailsWrapper& detailsWrapper,
	                                     const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		renderPass = createRenderPass(device, detailsWrapper.surfaceFormat.format,
		                              allocationCallbacks);
	}

	void RenderPassWrapper::recreate(const SwapchainDetailsWrapper& detailsWrapper)
	{
		device.destroyRenderPass(renderPass, allocationCallbacks);
		renderPass = createRenderPass(device, detailsWrapper.surfaceFormat.format,
		                              allocationCallbacks);
	}

	RenderPassWrapper::~RenderPassWrapper()
	{
		device.destroyRenderPass(renderPass, allocationCallbacks);
	}

	DescriptorSetLayoutWrapper::DescriptorSetLayoutWrapper(const DeviceWrapper& deviceWrapper,
	                                                       const vk::DescriptorType& type,
	                                                       const u32 bindingIndex,
	                                                       const vk::ShaderStageFlags& stage,
	                                                       const AllocationCallbacksWrapper&
	                                                       allocationCallbacksWrapper): device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		descriptorSetLayout = createDescriptorSetLayout(device, type, bindingIndex,stage,
		                                                allocationCallbacks);
	}

	DescriptorSetLayoutWrapper::~DescriptorSetLayoutWrapper()
	{
		device.destroyDescriptorSetLayout(descriptorSetLayout, allocationCallbacks);
	}

	DescriptorPoolWrapper::DescriptorPoolWrapper(const DeviceWrapper& deviceWrapper, const vk::DescriptorType& type,
	                                             const u32 size,
	                                             const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		descriptorPool = spite::createDescriptorPool(device, allocationCallbacks, type,
		                                             size);
	}

	DescriptorPoolWrapper::~DescriptorPoolWrapper()
	{
		device.destroyDescriptorPool(descriptorPool, allocationCallbacks);
	}

	DescriptorSetsWrapper::DescriptorSetsWrapper(const DeviceWrapper& deviceWrapper,
	                                             const DescriptorSetLayoutWrapper& descriptorSetLayoutWrapper,
	                                             const DescriptorPoolWrapper& descriptorPoolWrapper,
	                                             const spite::HeapAllocator& allocator,
	                                             const AllocationCallbacksWrapper& allocationCallbacksWrapper,
	                                             const u32 count, const u32 bindingIndex,
	                                             const BufferWrapper& bufferWrapper,
	                                             const sizet bufferElementSize):
		dynamicOffset(static_cast<u32>(bufferElementSize)),
		descriptorPool(descriptorPoolWrapper.descriptorPool),
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		descriptorSets = spite::createDescriptorSets(device, descriptorSetLayoutWrapper.descriptorSetLayout,
		                                             descriptorPool, allocator,
		                                             allocationCallbacks, count);
		for (u32 i = 0; i < count; ++i)
		{
			spite::updateDescriptorSets(device, descriptorSets[i], bufferWrapper.buffer,
			                            vk::DescriptorType::eUniformBufferDynamic,
			                            bindingIndex,
			                            bufferElementSize);
		}
	}

	DescriptorSetsWrapper::~DescriptorSetsWrapper()
	{
		device.freeDescriptorSets(descriptorPool, descriptorSets);
		descriptorSets.clear();
	}

	ShaderModuleWrapper::ShaderModuleWrapper(const DeviceWrapper& deviceWrapper,
	                                         const eastl::vector<char, spite::HeapAllocator>& code,
	                                         const vk::ShaderStageFlagBits& stageFlag,
	                                         const AllocationCallbacksWrapper& allocationCallbacksWrapper) :
		stage(stageFlag), device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		shaderModule = createShaderModule(device, code, allocationCallbacks);
	}

	ShaderModuleWrapper& ShaderModuleWrapper::operator=(ShaderModuleWrapper&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		shaderModule = other.shaderModule;
		stage = other.stage;
		device = other.device;
		allocationCallbacks = other.allocationCallbacks;

		other.shaderModule = nullptr;
		other.device = nullptr;
		other.allocationCallbacks = nullptr;

		return *this;
	}

	ShaderModuleWrapper::ShaderModuleWrapper(ShaderModuleWrapper&& other) noexcept: shaderModule(other.shaderModule),
		stage(other.stage),
		device(other.device),
		allocationCallbacks(other.allocationCallbacks)
	{
		other.shaderModule = nullptr;
		other.device = nullptr;
		other.allocationCallbacks = nullptr;
	}

	ShaderModuleWrapper::~ShaderModuleWrapper()
	{
		if (device)
		{
			device.destroyShaderModule(shaderModule, allocationCallbacks);
		}
	}

	VertexInputDescriptionsWrapper::VertexInputDescriptionsWrapper(
		const eastl::vector<vk::VertexInputBindingDescription, spite::HeapAllocator>& bindingDescriptions,
		const eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator>& attributeDescriptions) :
		bindingDescriptions(bindingDescriptions), attributeDescriptions(attributeDescriptions)
	{
	}

	GraphicsPipelineWrapper::GraphicsPipelineWrapper(const DeviceWrapper& deviceWrapper,
	                                                 const eastl::vector<
		                                                 DescriptorSetLayoutWrapper*, spite::HeapAllocator>&
	                                                 descriptorSetLayouts,
	                                                 const SwapchainDetailsWrapper& detailsWrapper,
	                                                 const RenderPassWrapper& renderPassWrapper,
	                                                 const spite::HeapAllocator& allocator,
	                                                 const eastl::vector<
		                                                 eastl::tuple<ShaderModuleWrapper&, const char*>,
		                                                 spite::HeapAllocator>& shaderModules,
	                                                 const VertexInputDescriptionsWrapper& vertexInputDescription,
	                                                 const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		shaderStages(allocator),
		vertexInputInfo(
			{},
			static_cast<u32>(vertexInputDescription.bindingDescriptions.size()),
			vertexInputDescription.bindingDescriptions.data(),
			static_cast<u32>(vertexInputDescription.attributeDescriptions.size()),
			vertexInputDescription.attributeDescriptions.data()),
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		shaderStages.reserve(shaderModules.size());

		for (auto shaderModule : shaderModules)
		{
			vk::PipelineShaderStageCreateInfo createInfo(
				{},
				eastl::get<0>(shaderModule).stage,
				eastl::get<0>(shaderModule).shaderModule,
				eastl::get<1>(shaderModule));
			shaderStages.push_back(createInfo);
		}

		std::vector<vk::DescriptorSetLayout> layouts(descriptorSetLayouts.size());
		for (sizet i = 0, size = layouts.size(); i < size; ++i)
		{
			layouts[i] = descriptorSetLayouts[i]->descriptorSetLayout;
		}

		pipelineLayout = createPipelineLayout(device, layouts, allocationCallbacks);

		graphicsPipeline = createGraphicsPipeline(device, pipelineLayout,
		                                          detailsWrapper.extent, renderPassWrapper.renderPass, shaderStages,
		                                          vertexInputInfo,
		                                          allocationCallbacks);
	}

	GraphicsPipelineWrapper::GraphicsPipelineWrapper(GraphicsPipelineWrapper&& other) noexcept:
		pipelineLayout(other.pipelineLayout),
		graphicsPipeline(other.graphicsPipeline),
		shaderStages(std::move(other.shaderStages)),
		vertexInputInfo(other.vertexInputInfo),
		device(other.device),
		allocationCallbacks(other.allocationCallbacks)
	{
		other.device = nullptr;
	}

	GraphicsPipelineWrapper& GraphicsPipelineWrapper::operator=(GraphicsPipelineWrapper&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		pipelineLayout = other.pipelineLayout;
		graphicsPipeline = other.graphicsPipeline;
		shaderStages = std::move(other.shaderStages);
		vertexInputInfo = other.vertexInputInfo;
		device = other.device;
		allocationCallbacks = other.allocationCallbacks;


		other.device = nullptr;
		return *this;
	}

	void GraphicsPipelineWrapper::recreate(const SwapchainDetailsWrapper& detailsWrapper,
	                                       const RenderPassWrapper& renderPassWrapper)
	{
		device.destroyPipeline(graphicsPipeline, allocationCallbacks);

		graphicsPipeline = createGraphicsPipeline(device, pipelineLayout,
		                                          detailsWrapper.extent, renderPassWrapper.renderPass, shaderStages,
		                                          vertexInputInfo,
		                                          allocationCallbacks);
	}

	GraphicsPipelineWrapper::~GraphicsPipelineWrapper()
	{
		if (!device)
		{
			return;
		}
		device.destroyPipeline(graphicsPipeline, allocationCallbacks);
		device.destroyPipelineLayout(pipelineLayout, allocationCallbacks);
	}

	FramebuffersWrapper::FramebuffersWrapper(const DeviceWrapper& deviceWrapper,
	                                         const spite::HeapAllocator& allocator,
	                                         const ImageViewsWrapper& imageViewsWrapper,
	                                         const SwapchainDetailsWrapper& detailsWrapper,
	                                         const RenderPassWrapper& renderPassWrapper,
	                                         const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		framebuffers(allocator),
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		framebuffers = createFramebuffers(device, allocator, imageViewsWrapper.imageViews,
		                                  detailsWrapper.extent,
		                                  renderPassWrapper.renderPass,
		                                  allocationCallbacks);
	}

	void FramebuffersWrapper::recreate(const SwapchainDetailsWrapper& detailsWrapper,
	                                   const ImageViewsWrapper& imageViewsWrapper,
	                                   const RenderPassWrapper& renderPassWrapper)
	{
		for (const auto& framebuffer : framebuffers)
		{
			device.destroyFramebuffer(framebuffer, allocationCallbacks);
		}
		framebuffers.clear();

		framebuffers = createFramebuffers(device, framebuffers.get_allocator(), imageViewsWrapper.imageViews,
		                                  detailsWrapper.extent,
		                                  renderPassWrapper.renderPass,
		                                  allocationCallbacks);
	}

	FramebuffersWrapper::~FramebuffersWrapper()
	{
		for (const auto& framebuffer : framebuffers)
		{
			device.destroyFramebuffer(framebuffer, allocationCallbacks);
		}
	}

	CommandPoolWrapper::CommandPoolWrapper(const DeviceWrapper& deviceWrapper, const u32 familyIndex,
	                                       const vk::CommandPoolCreateFlagBits& flagBits,
	                                       const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		commandPool = createCommandPool(device, allocationCallbacks, flagBits,
		                                familyIndex);
	}

	CommandPoolWrapper::~CommandPoolWrapper()
	{
		device.destroyCommandPool(commandPool, allocationCallbacks);
	}

	BufferWrapper::BufferWrapper(const u64 size, const vk::BufferUsageFlags& usage,
	                             const vk::MemoryPropertyFlags memoryProperty,
	                             const vma::AllocationCreateFlags& allocationFlag, const QueueFamilyIndices& indices,
	                             const GpuAllocatorWrapper& allocatorWrapper): allocator(allocatorWrapper.allocator),
	                                                                           size(size)
	{
		createBuffer(size, usage, memoryProperty, allocationFlag, indices, allocator, buffer,
		             allocation);
	}

	BufferWrapper::BufferWrapper(BufferWrapper&& other) noexcept:
		buffer(other.buffer),
		allocation(other.allocation),
		allocator(other.allocator),
		size(other.size)
	{
		other.allocator = nullptr;
		other.buffer = nullptr;
		other.allocation = nullptr;
	}

	BufferWrapper& BufferWrapper::operator=(BufferWrapper&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		buffer = other.buffer;
		allocation = other.allocation;
		allocator = other.allocator;
		size = other.size;

		other.allocator = nullptr;
		other.buffer = nullptr;
		other.allocation = nullptr;

		return *this;
	}

	void BufferWrapper::copyBuffer(const BufferWrapper& other, const vk::Device& device, const vk::CommandPool&
	                               transferCommandPool, const vk::Queue transferQueue,
	                               const vk::AllocationCallbacks* allocationCallbacks) const
	{
		SASSERTM(size == other.size, "Buffer sizes are not equal")
		spite::copyBuffer(other.buffer, buffer, size, transferCommandPool, device, transferQueue, allocationCallbacks);
	}

	void BufferWrapper::copyMemory(const void* data, const vk::DeviceSize& memorySize,
	                               const vk::DeviceSize& localOffset) const
	{
		vk::Result result = allocator.copyMemoryToAllocation(data, allocation, localOffset, memorySize);
		SASSERT_VULKAN(result)
	}

	void* BufferWrapper::mapMemory() const
	{
		auto [result,memory] = allocator.mapMemory(allocation);
		SASSERT_VULKAN(result)
		return memory;
	}

	void BufferWrapper::unmapMemory() const
	{
		allocator.unmapMemory(allocation);
	}

	BufferWrapper::~BufferWrapper()
	{
		if (allocator)
			allocator.destroyBuffer(buffer, allocation);
	}

	CommandBuffersWrapper::CommandBuffersWrapper(const DeviceWrapper& deviceWrapper,
	                                             const CommandPoolWrapper& commandPoolWrapper,
	                                             const vk::CommandBufferLevel& level, const u32 count):
		commandPool(commandPoolWrapper.commandPool),
		device(deviceWrapper.device)
	{
		commandBuffers = createGraphicsCommandBuffers(device, commandPool, level, count);
	}

	CommandBuffersWrapper::CommandBuffersWrapper(CommandBuffersWrapper&& other) noexcept:
		commandBuffers(std::move(other.commandBuffers)),
		commandPool(other.commandPool),
		device(other.device)
	{
		other.device = nullptr;
		other.commandPool = nullptr;
	}

	CommandBuffersWrapper& CommandBuffersWrapper::operator=(CommandBuffersWrapper&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		commandBuffers = std::move(other.commandBuffers);
		commandPool = other.commandPool;
		device = other.device;

		other.device = nullptr;
		other.commandPool = nullptr;

		return *this;
	}

	CommandBuffersWrapper::~CommandBuffersWrapper()
	{
		if (device)
			device.freeCommandBuffers(commandPool, static_cast<u32>(commandBuffers.size()),
			                          commandBuffers.data());
	}

	SyncObjectsWrapper::SyncObjectsWrapper(const DeviceWrapper& deviceWrapper, const u32 count,
	                                       const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		imageAvailableSemaphores.resize(count);
		renderFinishedSemaphores.resize(count);
		inFlightFences.resize(count);

		vk::SemaphoreCreateInfo semaphoreInfo;
		vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

		for (size_t i = 0; i < count; ++i)
		{
			imageAvailableSemaphores[i] = createSemaphore(device, semaphoreInfo,
			                                              allocationCallbacks);
			renderFinishedSemaphores[i] = createSemaphore(device, semaphoreInfo,
			                                              allocationCallbacks);
			inFlightFences[i] = createFence(device, fenceInfo,
			                                allocationCallbacks);
		}
	}

	SyncObjectsWrapper::~SyncObjectsWrapper()
	{
		for (size_t i = 0; i < inFlightFences.size(); ++i)
		{
			device.destroySemaphore(imageAvailableSemaphores[i],
			                        allocationCallbacks);
			device.destroySemaphore(renderFinishedSemaphores[i],
			                        allocationCallbacks);
			device.destroyFence(inFlightFences[i], allocationCallbacks);
		}
	}
}
