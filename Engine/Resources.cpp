#include "Resources.hpp"

#include "Core.hpp"
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
			&vkAllocationCallback,
			&vkFreeCallback);
	}

	AllocationCallbacksWrapper::~AllocationCallbacksWrapper()
	{
		allocationCallbacks.pUserData = nullptr;
	}

	InstanceExtensions::InstanceExtensions(char const* const* windowExtensions, const u32 windowExtensionsCount,
	                                       const spite::HeapAllocator& allocator)
	{
		extensions = getRequiredExtensions(allocator, windowExtensions, windowExtensionsCount);
	}

	InstanceWrapper::InstanceWrapper(const spite::HeapAllocator& allocator,
	                                 const AllocationCallbacksWrapper& allocationCallbacksWrapper,
	                                 const eastl::vector<const char*, spite::HeapAllocator>& extensions):
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		instance = spite::createInstance(allocator, *allocationCallbacks, extensions);
	}

	InstanceWrapper::~InstanceWrapper()
	{
		instance.destroy(allocationCallbacks);
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
	                                         const vk::AllocationCallbacks& allocationCallbacksWrapper)
	{
		allocator = spite::createVmAllocator(physicalDevice.device, deviceWrapper.device, instanceWrapper.instance,
		                                     &allocationCallbacksWrapper);
	}

	GpuAllocatorWrapper::~GpuAllocatorWrapper()
	{
		allocator.destroy();
	}

	SwapchainDetailsWrapper::SwapchainDetailsWrapper(const PhysicalDeviceWrapper& physicalDeviceWrapper,
	                                                 const vk::SurfaceKHR& surface,
	                                                 const int width, const int height): surface(surface)
	{
		supportDetails = querySwapchainSupport(physicalDeviceWrapper.device, surface);
		surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
		presentMode = chooseSwapPresentMode(supportDetails.presentModes);
		extent = chooseSwapExtent(supportDetails.capabilities, width, height);
	}

	SwapchainWrapper::SwapchainWrapper(const PhysicalDeviceWrapper& physicalDeviceWrapper,
	                                   const DeviceWrapper& deviceWrapper,
	                                   const SwapchainDetailsWrapper& swapchainDetailsWrapper,
	                                   const AllocationCallbacksWrapper& allocationCallbacksWrapper,
	                                   const spite::HeapAllocator& allocator): device(deviceWrapper.device),
	                                                                           allocationCallbacks(
		                                                                           &allocationCallbacksWrapper.
		                                                                           allocationCallbacks)

	{
		swapchain = spite::createSwapchain(physicalDeviceWrapper.device, swapchainDetailsWrapper.surface,
		                                   swapchainDetailsWrapper.supportDetails,
		                                   device, swapchainDetailsWrapper.extent,
		                                   swapchainDetailsWrapper.surfaceFormat, swapchainDetailsWrapper.presentMode,
		                                   allocator,
		                                   allocationCallbacks);
	}

	SwapchainWrapper::~SwapchainWrapper()
	{
		device.destroySwapchainKHR(swapchain, allocationCallbacks);
	}

	SwapchainImages::SwapchainImages(const DeviceWrapper& deviceWrapper,
	                                 const SwapchainWrapper& swapchainWrapper)
	{
		images = getSwapchainImages(deviceWrapper.device, swapchainWrapper.swapchain);
	}

	SwapchainImages::~SwapchainImages() = default;

	ImageViewsWrapper::ImageViewsWrapper(const DeviceWrapper& deviceWrapper,
	                                     const SwapchainImages& swapchainImagesWrapper,
	                                     const SwapchainDetailsWrapper& detailsWrapper,
	                                     const AllocationCallbacksWrapper& resourceAllocationWrapper):
		device(deviceWrapper.device),
		allocationCallbacks(&resourceAllocationWrapper.allocationCallbacks)
	{
		imageViews = createImageViews(device, swapchainImagesWrapper.images, detailsWrapper.surfaceFormat.format,
		                              allocationCallbacks);
	}

	ImageViewsWrapper::~ImageViewsWrapper()
	{
		for (const auto& imageView : imageViews)
		{
			device.destroyImageView(imageView, allocationCallbacks);
		}
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

	RenderPassWrapper::~RenderPassWrapper()
	{
		device.destroyRenderPass(renderPass, allocationCallbacks);
	}

	DescriptorSetLayoutWrapper::DescriptorSetLayoutWrapper(const DeviceWrapper& deviceWrapper,
	                                                       const AllocationCallbacksWrapper&
	                                                       allocationCallbacksWrapper): device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		descriptorSetLayout = createDescriptorSetLayout(device,
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
		descriptorPool = createDescriptorPool(device, allocationCallbacks, type,
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
	                                             const u32 count): descriptorPool(descriptorPoolWrapper.descriptorPool),
	                                                               device(deviceWrapper.device),
	                                                               allocationCallbacks(
		                                                               &allocationCallbacksWrapper.allocationCallbacks)
	{
		descriptorSets = createDescriptorSets(device, descriptorSetLayoutWrapper.descriptorSetLayout,
		                                      descriptorPool, count, allocator,
		                                      allocationCallbacks);
	}

	DescriptorSetsWrapper::~DescriptorSetsWrapper()
	{
		device.freeDescriptorSets(descriptorPool, descriptorSets,
		                          &allocationCallbacks);
	}

	ShaderModuleWrapper::ShaderModuleWrapper(const DeviceWrapper& deviceWrapper, const std::vector<char>& code,
	                                         const vk::ShaderStageFlagBits& stageFlag,
	                                         const AllocationCallbacksWrapper& allocationCallbacksWrapper) :
		stage(stageFlag), device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		shaderModule = createShaderModule(device, code, allocationCallbacks);
	}

	ShaderModuleWrapper::~ShaderModuleWrapper()
	{
		device.destroyShaderModule(shaderModule, allocationCallbacks);
	}

	VertexInputDescriptions::VertexInputDescriptions(
		eastl::vector<vk::VertexInputBindingDescription, spite::HeapAllocator> bindingDescriptions,
		eastl::vector<vk::VertexInputAttributeDescription, spite::HeapAllocator> attributeDescriptions) :
		bindingDescriptions(std::move(bindingDescriptions)), attributeDescriptions(std::move(attributeDescriptions))
	{
	}

	GraphicsPipelineWrapper::GraphicsPipelineWrapper(const DeviceWrapper& deviceWrapper,
	                                                 const DescriptorSetLayoutWrapper& descriptorSetLayoutWrapper,
	                                                 const SwapchainDetailsWrapper& detailsWrapper,
	                                                 const RenderPassWrapper& renderPassWrapper,
	                                                 const spite::HeapAllocator& allocator,
	                                                 eastl::array<std::tuple<
		                                                 std::shared_ptr<ShaderModuleWrapper>, const char*>>&
	                                                 shaderModules,
	                                                 const VertexInputDescriptions& vertexInputDescription,
	                                                 const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		eastl::vector<vk::PipelineShaderStageCreateInfo, spite::HeapAllocator> shaderStages(allocator);
		shaderStages.reserve(shaderModules.size());

		for (auto shaderModule : shaderModules)
		{
			vk::PipelineShaderStageCreateInfo createInfo(
				{},
				std::get<0>(shaderModule)->stage,
				std::get<0>(shaderModule)->shaderModule,
				std::get<1>(shaderModule));
			shaderStages.push_back(createInfo);
		}

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
			{},
			static_cast<u32>(vertexInputDescription.bindingDescriptions.size()),
			vertexInputDescription.bindingDescriptions.data(),
			static_cast<u32>(vertexInputDescription.attributeDescriptions.size()),
			vertexInputDescription.attributeDescriptions.data());

		graphicsPipeline = createGraphicsPipeline(device, descriptorSetLayoutWrapper.descriptorSetLayout,
		                                          detailsWrapper.extent, renderPassWrapper.renderPass, shaderStages,
		                                          vertexInputInfo,
		                                          allocationCallbacks);
	}

	GraphicsPipelineWrapper::~GraphicsPipelineWrapper()
	{
		device.destroyPipeline(graphicsPipeline, allocationCallbacks);
	}

	FramebuffersWrapper::FramebuffersWrapper(const DeviceWrapper& deviceWrapper,
	                                         const spite::HeapAllocator& allocator,
	                                         const ImageViewsWrapper& imageViewsWrapper,
	                                         const SwapchainDetailsWrapper& detailsWrapper,
	                                         const RenderPassWrapper& renderPassWrapper,
	                                         const AllocationCallbacksWrapper& allocationCallbacksWrapper):
		device(deviceWrapper.device),
		allocationCallbacks(&allocationCallbacksWrapper.allocationCallbacks)
	{
		framebuffers = createFramebuffers(device, allocator, imageViewsWrapper.imageViews,
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
	                             const GpuAllocatorWrapper& allocatorWrapper): allocator(allocatorWrapper.allocator)
	{
		createBuffer(size, usage, memoryProperty, allocationFlag, indices, allocator, buffer,
		             allocation);
	}

	BufferWrapper::~BufferWrapper()
	{
		allocator.destroyBuffer(buffer, allocation);
	}

	CommandBuffersWrapper::CommandBuffersWrapper(const DeviceWrapper& deviceWrapper,
	                                             const CommandPoolWrapper& commandPoolWrapper, const u32 count):
		commandPool(commandPoolWrapper.commandPool),
		device(deviceWrapper.device)
	{
		commandBuffers = createGraphicsCommandBuffers(device, commandPool, count);
	}

	CommandBuffersWrapper::~CommandBuffersWrapper()
	{
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
