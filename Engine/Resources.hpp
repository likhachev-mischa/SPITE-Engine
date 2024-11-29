#pragma once
//#include <vulkan/vulkan_enums.hpp>
//#include <vulkan-memory-allocator-hpp/vk_mem_alloc_enums.hpp>

#include "Engine/EngineCommon.hpp"
#include "Engine/EngineCore.hpp"
#include "Engine/VulkanAllocator.hpp"

namespace spite
{
	struct ResourceAllocationCallbacks
	{
		ResourceAllocationCallbacks(const ResourceAllocationCallbacks& other) = delete;
		ResourceAllocationCallbacks(ResourceAllocationCallbacks&& other) = delete;
		ResourceAllocationCallbacks& operator=(const ResourceAllocationCallbacks& other) = delete;
		ResourceAllocationCallbacks& operator=(ResourceAllocationCallbacks&& other) = delete;

		vk::AllocationCallbacks allocationCallbacks;

		explicit ResourceAllocationCallbacks(spite::HeapAllocator& allocator)
		{
			allocationCallbacks = vk::AllocationCallbacks(
				&allocator,
				&vkAllocate,
				&vkReallocate,
				&vkFree,
				&vkAllocationCallback,
				&vkFreeCallback);
		}

		~ResourceAllocationCallbacks()
		{
			allocationCallbacks.pUserData = nullptr;
		}
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
		         const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation,
		         const eastl::vector<const char*, spite::HeapAllocator>& extensions): pResourceAllocation(
			resourceAllocation)
		{
			instance = spite::createInstance(allocator, &pResourceAllocation->allocationCallbacks, extensions);
		}

		~Instance()
		{
			instance.destroy(&pResourceAllocation->allocationCallbacks);
		}
	};

	struct PhysicalDevice
	{
		vk::PhysicalDevice device;

		explicit PhysicalDevice(const Instance& instance)
		{
			device = spite::getPhysicalDevice(instance.instance);
		}
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
		       const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pResourceAllocation(
			resourceAllocation)
		{
			device = spite::createDevice(indices, physicalDevice.device, allocator,
			                             &pResourceAllocation->allocationCallbacks);
		}

		~Device()
		{
			device.destroy(&pResourceAllocation->allocationCallbacks);
		}
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
		             const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation)
		{
			allocator = spite::createVmAllocator(physicalDevice.device, device.device, instance.instance,
			                                     &resourceAllocation->allocationCallbacks);;
		}

		~GpuAllocator()
		{
			allocator.destroy();
		}
	};

	struct SwapchainDetails
	{
		SwapchainSupportDetails supportDetails;
		vk::SurfaceKHR surface;
		vk::SurfaceFormatKHR surfaceFormat;
		vk::PresentModeKHR presentMode{};
		vk::Extent2D extent;

		SwapchainDetails(const PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface, const int width,
		                 const int height): surface(surface)
		{
			supportDetails = querySwapchainSupport(physicalDevice.device, surface);
			surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
			presentMode = chooseSwapPresentMode(supportDetails.presentModes);
			extent = chooseSwapExtent(supportDetails.capabilities, width, height);
		}
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


		Swapchain(const PhysicalDevice& physicalDevice, const std::shared_ptr<Device>& device,
		          const SwapchainDetails& details,
		          const spite::HeapAllocator& allocator,
		          const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation): pDevice(device),
			pResourceAllocation(resourceAllocation)
		{
			swapchain = spite::createSwapchain(physicalDevice.device, details.surface, details.supportDetails,
			                                   device->device, details.extent,
			                                   details.surfaceFormat, details.presentMode, allocator,
			                                   &pResourceAllocation->allocationCallbacks);
		}

		~Swapchain()
		{
			pDevice->device.destroySwapchainKHR(swapchain, &pResourceAllocation->allocationCallbacks);
		}
	};

	struct SwapchainImages
	{
		SwapchainImages(const SwapchainImages& other) = delete;
		SwapchainImages(SwapchainImages&& other) = delete;
		SwapchainImages& operator=(const SwapchainImages& other) = delete;
		SwapchainImages& operator=(SwapchainImages&& other) = delete;

		std::vector<vk::Image> images{};

		SwapchainImages(const Device& device, const Swapchain& swapchain)
		{
			images = getSwapchainImages(device.device, swapchain.swapchain);
		}

		~SwapchainImages() = default;
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

		ImageViews(const std::shared_ptr<Device>& device, const SwapchainImages& swapchainImages,
		           const SwapchainDetails& details,
		           const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pDevice(device),
			pResourceAllocation(resourceAllocation)
		{
			imageViews = createImageViews(device->device, swapchainImages.images, details.surfaceFormat.format,
			                              &resourceAllocation->allocationCallbacks);
		}

		~ImageViews()
		{
			for (const auto& imageView : imageViews)
			{
				pDevice->device.destroyImageView(imageView, &pResourceAllocation->allocationCallbacks);
			}
		}
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

		RenderPass(const std::shared_ptr<Device>& device, const SwapchainDetails& details,
		           const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pDevice(device),
			pResourceAllocation(resourceAllocation)
		{
			renderPass = createRenderPass(device->device, details.surfaceFormat.format,
			                              &resourceAllocation->allocationCallbacks);
		}

		~RenderPass()
		{
			pDevice->device.destroyRenderPass(renderPass, &pResourceAllocation->allocationCallbacks);
		}
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

		DescriptorSetLayout(const std::shared_ptr<Device>& device,
		                    const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pDevice(device),
			pResourceAllocation(resourceAllocation)
		{
			descriptorSetLayout = createDescriptorSetLayout(device->device, &resourceAllocation->allocationCallbacks);
		}

		~DescriptorSetLayout()
		{
			pDevice->device.destroyDescriptorSetLayout(descriptorSetLayout, &pResourceAllocation->allocationCallbacks);
		}
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

		DescriptorPool(const std::shared_ptr<Device>& device, const vk::DescriptorType& type, const u32 size,
		               const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pDevice(device),
			pResourceAllocation(resourceAllocation)
		{
			descriptorPool = createDescriptorPool(device->device, &resourceAllocation->allocationCallbacks, type, size);
		}

		~DescriptorPool()
		{
			pDevice->device.destroyDescriptorPool(descriptorPool, &pResourceAllocation->allocationCallbacks);
		}
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

		DescriptorSets(const std::shared_ptr<Device>& device, const DescriptorSetLayout& descriptorSetLayout,
		               const DescriptorPool& descriptorPool, const spite::HeapAllocator& allocator,
		               const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation,
		               const u32 count) : pDevice(device),
		                                  pResourceAllocation(resourceAllocation)
		{
			descriptorSets = createDescriptorSets(device->device, descriptorSetLayout.descriptorSetLayout,
			                                      descriptorPool.descriptorPool, count, allocator,
			                                      &resourceAllocation->allocationCallbacks);
		}

		~DescriptorSets() = default;
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

		GraphicsPipeline(const std::shared_ptr<Device>& device, const DescriptorSetLayout& descriptorSetLayout,
		                 const SwapchainDetails& details, const RenderPass& renderPass,
		                 const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pDevice(device),
			pResourceAllocation(resourceAllocation)
		{
			graphicsPipeline = createGraphicsPipeline(device->device, descriptorSetLayout.descriptorSetLayout,
			                                          details.extent, renderPass.renderPass,
			                                          &resourceAllocation->allocationCallbacks);
		}

		~GraphicsPipeline()
		{
			pDevice->device.destroyPipeline(graphicsPipeline, &pResourceAllocation->allocationCallbacks);
		}
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

		Framebuffers(const std::shared_ptr<Device>& device, const spite::HeapAllocator& allocator,
		             const ImageViews& imageViews, const SwapchainDetails& details, const RenderPass& renderPass,
		             const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pDevice(device),
			pResourceAllocation(resourceAllocation)
		{
			framebuffers = createFramebuffers(device->device, allocator, imageViews.imageViews, details.extent,
			                                  renderPass.renderPass,
			                                  &pResourceAllocation->allocationCallbacks);
		}

		~Framebuffers()
		{
			for (const auto& framebuffer : framebuffers)
			{
				pDevice->device.destroyFramebuffer(framebuffer, &pResourceAllocation->allocationCallbacks);
			}
		}

		struct CommandPool
		{
			CommandPool(const CommandPool& other) = delete;
			CommandPool(CommandPool&& other) = delete;
			CommandPool& operator=(const CommandPool& other) = delete;
			CommandPool& operator=(CommandPool&& other) = delete;

			vk::CommandPool commandPool;
			std::shared_ptr<Device> pDevice;
			std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

			CommandPool(const std::shared_ptr<Device>& device, const u32 familyIndex,
			            const vk::CommandPoolCreateFlagBits& flagBits,
			            const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pDevice(device),
				pResourceAllocation(resourceAllocation)
			{
				commandPool = createCommandPool(device->device, &resourceAllocation->allocationCallbacks, flagBits,
				                                familyIndex);
			}

			~CommandPool()
			{
				pDevice->device.destroyCommandPool(commandPool, &pResourceAllocation->allocationCallbacks);
			}
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
			       const std::shared_ptr<GpuAllocator>& allocator) : pAllocator(allocator)
			{
				createBuffer(size, usage, memoryProperty, allocationFlag, indices, allocator->allocator, buffer,
				             allocation);
			}

			~Buffer()
			{
				pAllocator->allocator.destroyBuffer(buffer, allocation);
			}
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
			std::shared_ptr<ResourceAllocationCallbacks> pResourceAllocation;

			CommandBuffers(const std::shared_ptr<Device>& device, const std::shared_ptr<CommandPool>& commandPool,
			               const u32 count,
			               const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) :
				pCommandPool(commandPool),
				pDevice(device),
				pResourceAllocation(resourceAllocation)
			{
				commandBuffers = createGraphicsCommandBuffers(device->device, commandPool->commandPool, count);
			}

			~CommandBuffers()
			{
				pDevice->device.freeCommandBuffers(pCommandPool->commandPool, commandBuffers.size(),
				                                   commandBuffers.data());
			}
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

			SyncObjects(const std::shared_ptr<Device>& device, const u32 count,
			            const std::shared_ptr<ResourceAllocationCallbacks>& resourceAllocation) : pDevice(device),
				pResourceAllocation(resourceAllocation)
			{
				imageAvailableSemaphores.resize(count);
				renderFinishedSemaphores.resize(count);
				inFlightFences.resize(count);

				vk::SemaphoreCreateInfo semaphoreInfo;
				vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

				for (size_t i = 0; i < count; ++i)
				{
					imageAvailableSemaphores[i] = createSemaphore(device->device, semaphoreInfo,
					                                              &resourceAllocation->allocationCallbacks);
					renderFinishedSemaphores[i] = createSemaphore(device->device, semaphoreInfo,
					                                              &resourceAllocation->allocationCallbacks);
					inFlightFences[i] = createFence(device->device, fenceInfo,
					                                &resourceAllocation->allocationCallbacks);
				}
			}

			~SyncObjects()
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
		};
	};
}
