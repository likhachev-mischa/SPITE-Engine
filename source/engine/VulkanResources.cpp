#include "VulkanResources.hpp"

#include <set>

#include "Common.hpp"
#include "VulkanDebug.hpp"

#include "Base/Assert.hpp"
#include "Base/Logging.hpp"

namespace spite
{
	std::vector<const char*> getRequiredExtensions(char const* const* windowExtensions,
	                                               const u32 windowExtensionCount)
	{
		std::vector<const char*> extensions(windowExtensions,
		                                    windowExtensions + windowExtensionCount);

		if (ENABLE_VALIDATION_LAYERS)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	vk::Instance createInstance(const vk::AllocationCallbacks& allocationCallbacks,
	                            const std::vector<const char*>& extensions)
	{
		if constexpr (ENABLE_VALIDATION_LAYERS)
		{
			SASSERTM(checkValidationLayerSupport(),
			         "Validation layers requested, but not available!")
		}

		vk::ApplicationInfo appInfo(APPLICATION_NAME,
		                            vk::makeApiVersion(0, 1, 0, 0),
		                            ENGINE_NAME,
		                            vk::makeApiVersion(0, 1, 0, 0));

		vk::InstanceCreateInfo createInfo({},
		                                  &appInfo,
		                                  {},
		                                  {},
		                                  static_cast<u32>(extensions.size()),
		                                  extensions.data(),
		                                  {});

		if constexpr (ENABLE_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<u32>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = createDebugMessengerCreateInfo();
			createInfo.pNext = &debugCreateInfo;
		}

		auto [result, instance] = vk::createInstance(createInfo, allocationCallbacks);
		SASSERT_VULKAN(result)
		return instance;
	}


	vk::PhysicalDevice getPhysicalDevice(const vk::Instance& instance)
	{
		auto [result, devices] = instance.enumeratePhysicalDevices();
		SASSERT_VULKAN(result)

		SASSERTM(!devices.empty(), "Failed to locate physical devices!")
		vk::PhysicalDevice physicalDevice;

		for (const auto& device : devices)
		{
			if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				physicalDevice = device;
				break;
			}
		}

		SASSERTM(physicalDevice != VK_NULL_HANDLE, "Failed to locate physical devices!")
		return physicalDevice;
	}

	vk::Device createDevice(const QueueFamilyIndices& indices,
	                        const vk::PhysicalDevice& physicalDevice,
	                        const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		std::set<u32> uniqueQueueFamilies = {
			indices.graphicsFamily.value(), indices.presentFamily.value(),
			indices.transferFamily.value()
		};

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		queueCreateInfos.reserve(uniqueQueueFamilies.size());

		float queuePriority = 1.0f;
		for (u32 queueFamily : uniqueQueueFamilies)
		{
			vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamily, 1, &queuePriority, {});
			queueCreateInfos.push_back(queueCreateInfo);
		}

		vk::PhysicalDeviceFeatures deviceFeatures{};

		vk::DeviceCreateInfo createInfo({},
		                                static_cast<uint32_t>(queueCreateInfos.size()),
		                                queueCreateInfos.data(),
		                                {},
		                                {},
		                                static_cast<uint32_t>(DEVICE_EXTENSIONS.size()),
		                                DEVICE_EXTENSIONS.data(),
		                                &deviceFeatures);

		if (ENABLE_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		}

		auto [result, device] = physicalDevice.createDevice(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)

		return device;
	}

	vma::Allocator createVmAllocator(const vk::PhysicalDevice& physicalDevice,
	                                 const vk::Device& device,
	                                 const vk::Instance& instance,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vma::AllocatorCreateInfo createInfo({},
		                                    physicalDevice,
		                                    device,
		                                    {},
		                                    pAllocationCallbacks,
		                                    {},
		                                    {},
		                                    {},
		                                    instance,
		                                    VK_API_VERSION);
		auto [result, allocator] = vma::createAllocator(createInfo);
		SASSERT_VULKAN(result)
		return allocator;
	}

	QueueFamilyIndices findQueueFamilies(const vk::SurfaceKHR& surface,
	                                     const vk::PhysicalDevice& physicalDevice)
	{
		QueueFamilyIndices indices;

		std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.
			getQueueFamilyProperties();

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics && !indices.graphicsFamily.
				has_value())
			{
				indices.graphicsFamily = i;
			}

			vk::Bool32 presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface).value;
			if (presentSupport && !indices.presentFamily.has_value() && i != indices.graphicsFamily)
			{
				indices.presentFamily = i;
			}
			//override presentFamily if there is distinct present queue available
			//if (presentSupport && i != indices.graphicsFamily)
			//{
			//	indices.presentFamily = i;
			//}
			if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer && i != indices.graphicsFamily
				&& i != indices.presentFamily)
			{
				indices.transferFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}
			i++;
		}
		if (!indices.presentFamily.has_value())
		{
			vk::Bool32 presentSupport = physicalDevice.getSurfaceSupportKHR(
				indices.graphicsFamily.value(),
				surface).value;
			SASSERT(presentSupport, "No present queues were found");

			indices.presentFamily = indices.graphicsFamily.value();
		}
		if (!indices.transferFamily.has_value())
		{
			indices.transferFamily = indices.graphicsFamily.value();
		}

		SASSERTM(indices.isComplete(), "Failed to find queue families!")
		return indices;
	}


	SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice physicalDevice,
	                                              const vk::SurfaceKHR surface)
	{
		SwapchainSupportDetails details;
		details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface).value;
		details.formats = physicalDevice.getSurfaceFormatsKHR(surface).value;
		details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface).value;
		return details;
	}


	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace ==
				vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return format;
			}
		}
		return availableFormats[0];
	}

	vk::PresentModeKHR chooseSwapPresentMode(
		const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		for (const auto& presentMode : availablePresentModes)
		{
			if (presentMode == vk::PresentModeKHR::eMailbox)
			{
				return presentMode;
			}
		}
		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
	                              int width,
	                              int height)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<u32>::max())
		{
			return capabilities.currentExtent;
		}
		vk::Extent2D actualExtent(static_cast<u32>(width), static_cast<u32>(height));
		actualExtent.width = std::clamp(actualExtent.width,
		                                capabilities.minImageExtent.width,
		                                capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height,
		                                 capabilities.minImageExtent.height,
		                                 capabilities.maxImageExtent.height);
		return actualExtent;
	}

	vk::SwapchainKHR createSwapchain(const vk::Device& device,
	                                 const QueueFamilyIndices& indices,
	                                 const vk::SurfaceKHR& surface,
	                                 const vk::SurfaceCapabilitiesKHR& capabilities,
	                                 const vk::Extent2D& extent,
	                                 const vk::SurfaceFormatKHR& surfaceFormat,
	                                 const vk::PresentModeKHR& presentMode,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		u32 imageCount = capabilities.minImageCount + 1;

		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}

		vk::SwapchainCreateInfoKHR createInfo({},
		                                      surface,
		                                      imageCount,
		                                      surfaceFormat.format,
		                                      surfaceFormat.colorSpace,
		                                      extent,
		                                      1,
		                                      vk::ImageUsageFlagBits::eColorAttachment,
		                                      {},
		                                      {},
		                                      {},
		                                      capabilities.currentTransform,
		                                      vk::CompositeAlphaFlagBitsKHR::eOpaque,
		                                      presentMode,
		                                      vk::True);


		u32 queueFamilyInidces[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if (indices.graphicsFamily.value() != indices.presentFamily.value())
		{
			createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyInidces;
		}
		else
		{
			createInfo.imageSharingMode = vk::SharingMode::eExclusive;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		auto [result, swapchain] = device.createSwapchainKHR(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return swapchain;
	}

	std::vector<vk::Image> getSwapchainImages(const vk::Device& device,
	                                          const vk::SwapchainKHR& swapchain)
	{
		auto [result, swapchainImages] = device.getSwapchainImagesKHR(swapchain);
		SASSERT_VULKAN(result)
		return swapchainImages;
	}

	std::vector<vk::ImageView> createSwapchainImageViews(const vk::Device& device,
	                                                     const std::vector<vk::Image>&
	                                                     swapchainImages,
	                                                     const vk::Format& imageFormat,
	                                                     const vk::AllocationCallbacks*
	                                                     pAllocationCallbacks)
	{
		std::vector<vk::ImageView> imageViews;
		imageViews.reserve(swapchainImages.size());
		for (const auto& image : swapchainImages)
		{
			vk::ImageViewCreateInfo createInfo({},
			                                   image,
			                                   vk::ImageViewType::e2D,
			                                   imageFormat,
			                                   {},
			                                   {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
			auto [result, imageView] = device.createImageView(createInfo, pAllocationCallbacks);
			SASSERT_VULKAN(result)
			imageViews.push_back(imageView);
		}
		return imageViews;
	}

	vk::DescriptorSetLayout createDescriptorSetLayout(const vk::Device& device,
	                                                  const vk::DescriptorType& type,
	                                                  const u32 bindingIndex,
	                                                  const vk::ShaderStageFlags& stage,
	                                                  const vk::AllocationCallbacks*
	                                                  pAllocationCallbacks)
	{
		vk::DescriptorSetLayoutBinding uboLayoutBinding(bindingIndex, type, 1, stage, {});

		vk::DescriptorSetLayoutCreateInfo layoutInfo({}, 1, &uboLayoutBinding);

		auto [result, descriptorSetLayout] = device.createDescriptorSetLayout(
			layoutInfo,
			pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return descriptorSetLayout;
	}

	vk::ShaderModule createShaderModule(const vk::Device& device,
	                                    const std::vector<char>& code,
	                                    const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::ShaderModuleCreateInfo createInfo({},
		                                      code.size(),
		                                      reinterpret_cast<const u32*>(code.data()));

		auto [result, shaderModule] = device.createShaderModule(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return shaderModule;
	}

	//uses createShaderModule
	vk::PipelineShaderStageCreateInfo createShaderStageInfo(const vk::Device& device,
	                                                        const std::vector<char>& code,
	                                                        const vk::ShaderStageFlagBits& stage,
	                                                        const char* name,
	                                                        const vk::AllocationCallbacks*
	                                                        pAllocationCallbacks)
	{
		vk::ShaderModule shaderModule = createShaderModule(device, code, pAllocationCallbacks);
		vk::PipelineShaderStageCreateInfo shaderStageInfo({}, stage, shaderModule, name);
		return shaderStageInfo;
	}


	vk::PipelineLayout createPipelineLayout(const vk::Device& device,
	                                        const std::vector<vk::DescriptorSetLayout>&
	                                        descriptorSetLayouts,
	                                        const u32 pushConstantSize,
	                                        const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::PushConstantRange pushConstant(vk::ShaderStageFlagBits::eVertex, 0, pushConstantSize);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo({},
		                                                descriptorSetLayouts.size(),
		                                                descriptorSetLayouts.data(),
		                                                1,
		                                                &pushConstant);

		auto [result, pipelineLayout] = device.createPipelineLayout(
			pipelineLayoutInfo,
			pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return pipelineLayout;
	}

	vk::CommandPool createCommandPool(const vk::Device& device,
	                                  const vk::AllocationCallbacks* pAllocationCallbacks,
	                                  const vk::CommandPoolCreateFlags& flags,
	                                  const u32 queueFamilyIndex)
	{
		vk::CommandPoolCreateInfo commandPoolCreateInfo(flags, queueFamilyIndex);
		auto [result, commandPool] = device.createCommandPool(
			commandPoolCreateInfo,
			pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return commandPool;
	}

	void createBuffer(const vk::DeviceSize& size,
	                  const vk::BufferUsageFlags& usage,
	                  const vk::MemoryPropertyFlags& properties,
	                  const vma::AllocationCreateFlags& allocationFlags,
	                  const QueueFamilyIndices& indices,
	                  const vma::Allocator& allocator,
	                  vk::Buffer& buffer,
	                  vma::Allocation& bufferMemory)
	{
		u32 queues[] = {indices.graphicsFamily.value(), indices.transferFamily.value()};

		vk::BufferCreateInfo bufferInfo;
		if (indices.graphicsFamily.value() != indices.transferFamily.value())
		{
			bufferInfo = vk::BufferCreateInfo({},
			                                  size,
			                                  usage,
			                                  vk::SharingMode::eConcurrent,
			                                  2,
			                                  queues);
		}
		else
		{
			bufferInfo = vk::BufferCreateInfo({},
			                                  size,
			                                  usage,
			                                  vk::SharingMode::eExclusive,
			                                  0,
			                                  nullptr);
		}

		vma::AllocationCreateInfo allocInfo(allocationFlags, vma::MemoryUsage::eAuto, properties);

		auto [result, bufferAllocation] = allocator.createBuffer(bufferInfo, allocInfo);
		SASSERT_VULKAN(result)

		buffer = bufferAllocation.first;
		bufferMemory = bufferAllocation.second;
	}

	void copyBuffer(const vk::Buffer& srcBuffer,
	                const vk::Buffer& dstBuffer,
	                const vk::DeviceSize& size,
	                const vk::CommandPool& transferCommandPool,
	                const vk::Device& device,
	                const vk::Queue& transferQueue,
	                const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::CommandBufferAllocateInfo allocInfo(transferCommandPool,
		                                        vk::CommandBufferLevel::ePrimary,
		                                        1);

		auto [result, commandBuffers] = device.allocateCommandBuffers(allocInfo);
		SASSERT_VULKAN(result)

		vk::CommandBuffer commandBuffer = commandBuffers[0];

		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		result = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(result)

		vk::BufferCopy copyRegion({}, {}, size);
		commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
		result = commandBuffer.end();
		SASSERT_VULKAN(result)

		vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer);

		result = transferQueue.submit({submitInfo});
		SASSERT_VULKAN(result)

		result = transferQueue.waitIdle();
		SASSERT_VULKAN(result)
		device.freeCommandBuffers(transferCommandPool, 1, &commandBuffer);
	}

	vk::DescriptorPool createDescriptorPool(const vk::Device& device,
	                                        const vk::AllocationCallbacks* pAllocationCallbacks,
	                                        const vk::DescriptorType& type,
	                                        const u32 size)
	{
		vk::DescriptorPoolSize poolSize(type, size);
		vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		                                      size,
		                                      1,
		                                      &poolSize);
		auto [result, descriptorPool] = device.createDescriptorPool(poolInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return descriptorPool;
	}

	std::vector<vk::DescriptorSet> createDescriptorSets(const vk::Device& device,
	                                                    const vk::DescriptorSetLayout&
	                                                    descriptorSetLayout,
	                                                    const vk::DescriptorPool& descriptorPool,
	                                                    const u32 count)
	{
		std::vector<vk::DescriptorSetLayout> layouts(count, descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo(descriptorPool, count, layouts.data());

		auto [result, descriptorSets] = device.allocateDescriptorSets(allocInfo);
		SASSERT_VULKAN(result)
		return descriptorSets;
	}

	void updateDescriptorSets(const vk::Device& device,
	                          const vk::DescriptorSet& descriptorSet,
	                          const vk::Buffer& buffer,
	                          const vk::DescriptorType& type,
	                          const u32 bindingIndex,
	                          const sizet bufferElementSize)
	{
		vk::DescriptorBufferInfo bufferInfo(buffer, 0, bufferElementSize);
		vk::WriteDescriptorSet descriptorWrite(descriptorSet,
		                                       bindingIndex,
		                                       0,
		                                       1,
		                                       type,
		                                       {},
		                                       &bufferInfo,
		                                       {});

		device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
	}

	std::vector<vk::CommandBuffer> createGraphicsCommandBuffers(const vk::Device& device,
	                                                            const vk::CommandPool&
	                                                            graphicsCommandPool,
	                                                            const vk::CommandBufferLevel& level,
	                                                            const u32 count)
	{
		vk::CommandBufferAllocateInfo allocInfo(graphicsCommandPool, level, count);

		auto [result, commandBuffers] = device.allocateCommandBuffers(allocInfo);
		SASSERT_VULKAN(result)

		return commandBuffers;
	}

	vk::Semaphore createSemaphore(const vk::Device device,
	                              const vk::SemaphoreCreateInfo& createInfo,
	                              const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		auto [result, semaphore] = device.createSemaphore(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return semaphore;
	}

	vk::Fence createFence(const vk::Device device,
	                      const vk::FenceCreateInfo& createInfo,
	                      const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		auto [result, fence] = device.createFence(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return fence;
	}
}
