#include "Resources.hpp"

#include "Core.hpp"
#include "Engine/VulkanAllocator.hpp"

namespace spite
{
	ResourceAllocationCallbacks::ResourceAllocationCallbacks(spite::HeapAllocator& allocator)
	{
		allocationCallbacks = vk::AllocationCallbacks(
			&allocator,
			&vkAllocate,
			&vkReallocate,
			&vkFree,
			&vkAllocationCallback,
			&vkFreeCallback);
	}

	ResourceAllocationCallbacks::~ResourceAllocationCallbacks()
	{
		allocationCallbacks.pUserData = nullptr;
	}

	Instance::Instance(const spite::HeapAllocator& allocator,
	                   std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation,
	                   const eastl::vector<const char*, spite::HeapAllocator>& extensions): pResourceAllocation(
		std::move(
			resourceAllocation))
	{
		instance = spite::createInstance(allocator, &pResourceAllocation->allocationCallbacks, extensions);
	}

	Instance::~Instance()
	{
		instance.destroy(&pResourceAllocation->allocationCallbacks);
	}

	PhysicalDevice::PhysicalDevice(const Instance& instance)
	{
		device = spite::getPhysicalDevice(instance.instance);
	}

	Device::Device(const PhysicalDevice& physicalDevice, const QueueFamilyIndices& indices,
	               const spite::HeapAllocator& allocator,
	               std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation): pResourceAllocation(std::move(
		resourceAllocation))
	{
		device = spite::createDevice(indices, physicalDevice.device, allocator,
		                             &pResourceAllocation->allocationCallbacks);
	}

	Device::~Device()
	{
		device.destroy(&pResourceAllocation->allocationCallbacks);
	}

	GpuAllocator::GpuAllocator(const PhysicalDevice& physicalDevice, const Device& device, const Instance& instance,
	                           const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation)
	{
		allocator = spite::createVmAllocator(physicalDevice.device, device.device, instance.instance,
		                                     &resourceAllocation->allocationCallbacks);;
	}

	GpuAllocator::~GpuAllocator()
	{
		allocator.destroy();
	}

	SwapchainDetails::SwapchainDetails(const PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface,
	                                   const int width, const int height): surface(surface)
	{
		supportDetails = querySwapchainSupport(physicalDevice.device, surface);
		surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
		presentMode = chooseSwapPresentMode(supportDetails.presentModes);
		extent = chooseSwapExtent(supportDetails.capabilities, width, height);
	}

	Swapchain::Swapchain(const PhysicalDevice& physicalDevice, std::shared_ptr<Device> device,
	                     const SwapchainDetails& details, const spite::HeapAllocator& allocator,
	                     std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation): pDevice(std::move(
			device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		swapchain = spite::createSwapchain(physicalDevice.device, details.surface, details.supportDetails,
		                                   pDevice->device, details.extent,
		                                   details.surfaceFormat, details.presentMode, allocator,
		                                   &pResourceAllocation->allocationCallbacks);
	}

	Swapchain::~Swapchain()
	{
		pDevice->device.destroySwapchainKHR(swapchain, &pResourceAllocation->allocationCallbacks);
	}

	SwapchainImages::SwapchainImages(const Device& device, const Swapchain& swapchain)
	{
		images = getSwapchainImages(device.device, swapchain.swapchain);
	}

	SwapchainImages::~SwapchainImages() = default;

	ImageViews::ImageViews(std::shared_ptr<Device> device, const SwapchainImages& swapchainImages,
	                       const SwapchainDetails& details,
	                       std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation): pDevice(std::move(device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		imageViews = createImageViews(pDevice->device, swapchainImages.images, details.surfaceFormat.format,
		                              &resourceAllocation->allocationCallbacks);
	}

	ImageViews::~ImageViews()
	{
		for (const auto& imageView : imageViews)
		{
			pDevice->device.destroyImageView(imageView, &pResourceAllocation->allocationCallbacks);
		}
	}

	RenderPass::RenderPass(std::shared_ptr<Device> device, const SwapchainDetails& details,
	                       std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation): pDevice(std::move(
			device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		renderPass = createRenderPass(pDevice->device, details.surfaceFormat.format,
		                              &pResourceAllocation->allocationCallbacks);
	}

	RenderPass::~RenderPass()
	{
		pDevice->device.destroyRenderPass(renderPass, &pResourceAllocation->allocationCallbacks);
	}

	DescriptorSetLayout::DescriptorSetLayout(std::shared_ptr<Device> device,
	                                         std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation): pDevice(
			std::move(device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		descriptorSetLayout = createDescriptorSetLayout(pDevice->device, &pResourceAllocation->allocationCallbacks);
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		pDevice->device.destroyDescriptorSetLayout(descriptorSetLayout, &pResourceAllocation->allocationCallbacks);
	}

	DescriptorPool::DescriptorPool(std::shared_ptr<Device> device, const vk::DescriptorType& type,
	                               const u32 size,
	                               std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation):
		pDevice(std::move(device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		descriptorPool = createDescriptorPool(pDevice->device, &pResourceAllocation->allocationCallbacks, type, size);
	}

	DescriptorPool::~DescriptorPool()
	{
		pDevice->device.destroyDescriptorPool(descriptorPool, &pResourceAllocation->allocationCallbacks);
	}

	DescriptorSets::DescriptorSets(std::shared_ptr<Device> device,
	                               const DescriptorSetLayout& descriptorSetLayout, const DescriptorPool& descriptorPool,
	                               const spite::HeapAllocator& allocator,
	                               std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation,
	                               const u32 count): pDevice(std::move(device)),
	                                                 pResourceAllocation(std::move(resourceAllocation))
	{
		descriptorSets = createDescriptorSets(pDevice->device, descriptorSetLayout.descriptorSetLayout,
		                                      descriptorPool.descriptorPool, count, allocator,
		                                      &pResourceAllocation->allocationCallbacks);
	}

	DescriptorSets::~DescriptorSets() = default;

	GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Device> device,
	                                   const DescriptorSetLayout& descriptorSetLayout, const SwapchainDetails& details,
	                                   const RenderPass& renderPass,
	                                   std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation):
		pDevice(std::move(device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		graphicsPipeline = createGraphicsPipeline(pDevice->device, descriptorSetLayout.descriptorSetLayout,
		                                          details.extent, renderPass.renderPass,
		                                          &pResourceAllocation->allocationCallbacks);
	}

	GraphicsPipeline::~GraphicsPipeline()
	{
		pDevice->device.destroyPipeline(graphicsPipeline, &pResourceAllocation->allocationCallbacks);
	}

	Framebuffers::Framebuffers(std::shared_ptr<Device> device, const spite::HeapAllocator& allocator,
	                           const ImageViews& imageViews, const SwapchainDetails& details,
	                           const RenderPass& renderPass,
	                           std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation): pDevice(
			std::move(device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		framebuffers = createFramebuffers(pDevice->device, allocator, imageViews.imageViews, details.extent,
		                                  renderPass.renderPass,
		                                  &pResourceAllocation->allocationCallbacks);
	}

	Framebuffers::~Framebuffers()
	{
		for (const auto& framebuffer : framebuffers)
		{
			pDevice->device.destroyFramebuffer(framebuffer, &pResourceAllocation->allocationCallbacks);
		}
	}

	CommandPool::CommandPool(std::shared_ptr<Device> device, const u32 familyIndex,
	                         const vk::CommandPoolCreateFlagBits& flagBits,
	                         std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation): pDevice(std::move(
			device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		commandPool = createCommandPool(pDevice->device, &pResourceAllocation->allocationCallbacks, flagBits,
		                                familyIndex);
	}

	CommandPool::~CommandPool()
	{
		pDevice->device.destroyCommandPool(commandPool, &pResourceAllocation->allocationCallbacks);
	}

	Buffer::Buffer(const u64 size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags memoryProperty,
	               const vma::AllocationCreateFlags& allocationFlag, const QueueFamilyIndices& indices,
	               std::shared_ptr<GpuAllocator> allocator): pAllocator(std::move(allocator))
	{
		createBuffer(size, usage, memoryProperty, allocationFlag, indices, pAllocator->allocator, buffer,
		             allocation);
	}

	Buffer::~Buffer()
	{
		pAllocator->allocator.destroyBuffer(buffer, allocation);
	}

	CommandBuffers::CommandBuffers(std::shared_ptr<Device> device,
	                               std::shared_ptr<CommandPool> commandPool, const u32 count):
		pCommandPool(std::move(commandPool)),
		pDevice(std::move(device))
	{
		commandBuffers = createGraphicsCommandBuffers(pDevice->device, pCommandPool->commandPool, count);
	}

	CommandBuffers::~CommandBuffers()
	{
		pDevice->device.freeCommandBuffers(pCommandPool->commandPool, commandBuffers.size(),
		                                   commandBuffers.data());
	}

	SyncObjects::SyncObjects(std::shared_ptr<Device> device, const u32 count,
	                         std::shared_ptr<ResourceAllocationCallbacks> resourceAllocation): pDevice(std::move(
			device)),
		pResourceAllocation(std::move(resourceAllocation))
	{
		imageAvailableSemaphores.resize(count);
		renderFinishedSemaphores.resize(count);
		inFlightFences.resize(count);

		vk::SemaphoreCreateInfo semaphoreInfo;
		vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

		for (size_t i = 0; i < count; ++i)
		{
			imageAvailableSemaphores[i] = createSemaphore(pDevice->device, semaphoreInfo,
			                                              &pResourceAllocation->allocationCallbacks);
			renderFinishedSemaphores[i] = createSemaphore(pDevice->device, semaphoreInfo,
			                                              &pResourceAllocation->allocationCallbacks);
			inFlightFences[i] = createFence(pDevice->device, fenceInfo,
			                                &pResourceAllocation->allocationCallbacks);
		}
	}

	SyncObjects::~SyncObjects()
	{
		for (size_t i = 0; i < inFlightFences.size(); ++i)
		{
			pDevice->device.destroySemaphore(imageAvailableSemaphores[i],
			                                 &pResourceAllocation->allocationCallbacks);
			pDevice->device.destroySemaphore(renderFinishedSemaphores[i],
			                                 &pResourceAllocation->allocationCallbacks);
			pDevice->device.destroyFence(inFlightFences[i], &pResourceAllocation->allocationCallbacks);
		}
	}
}
