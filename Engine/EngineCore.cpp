#include "EngineCore.hpp"

#include <set>
#include <EASTL/array.h>

#include"EngineCommon.hpp"
#include "EngineDebug.hpp"
#include "GraphicsUtility.hpp"
#include "Base/Assert.hpp"
#include "Base/Logging.hpp"

namespace spite
{
	eastl::vector<const char*, spite::HeapAllocator> getRequiredExtensions(const spite::HeapAllocator& allocator,
	                                                                       spite::WindowManager* windowManager)
	{
		uint32_t extensionCount = 0;
		char const* const* extensionNames = windowManager->getExtensions(extensionCount);

		eastl::vector<const char*, spite::HeapAllocator> extensions(extensionNames,
		                                                            extensionNames +
		                                                            extensionCount, allocator);

		if (ENABLE_VALIDATION_LAYERS)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	vk::Instance createInstance(const spite::HeapAllocator& allocator,
	                            const vk::AllocationCallbacks& allocationCallbacks, const
	                            eastl::vector<const char*, spite::HeapAllocator>& extensions)
	{
		if (ENABLE_VALIDATION_LAYERS)
		{
			SASSERTM(checkValidationLayerSupport(allocator), "Validation layers requested, but not available!")
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

		if (ENABLE_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<u32>(
				VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo =
				createDebugMessengerCreateInfo();
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
			if (device)
			{
				physicalDevice = device;
				break;
			}
		}

		SASSERTM(physicalDevice != VK_NULL_HANDLE, "Failed to locate physical devices!")
		return physicalDevice;
	}

	vk::Device createDevice(const QueueFamilyIndices& indices, const vk::PhysicalDevice& physicalDevice,
	                        const spite::HeapAllocator& allocator, const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		std::set<u32> uniqueQueueFamilies = {
			indices.graphicsFamily.value(), indices.presentFamily.value(),
			indices.transferFamily.value()
		};

		eastl::vector<vk::DeviceQueueCreateInfo, spite::HeapAllocator> queueCreateInfos(allocator);
		queueCreateInfos.reserve(uniqueQueueFamilies.size());

		float queuePriority = 1.0f;
		for (u32 queueFamily : uniqueQueueFamilies)
		{
			vk::DeviceQueueCreateInfo queueCreateInfo(
				{},
				queueFamily,
				1,
				&queuePriority,
				{});
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
			createInfo.enabledLayerCount = static_cast<uint32_t>(
				VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		}

		auto [result, device] = physicalDevice.createDevice(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)

		return device;
	}

	vma::Allocator createVmAllocator(const vk::PhysicalDevice& physicalDevice, const vk::Device& device,
	                                 const vk::Instance& instance, const vk::AllocationCallbacks* pAllocationCallbacks)
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
		auto [result,allocator] = vma::createAllocator(createInfo);
		SASSERT_VULKAN(result)
		return allocator;
	}

	QueueFamilyIndices findQueueFamilies(const vk::SurfaceKHR& surface, const vk::PhysicalDevice& physicalDevice,
	                                     const spite::HeapAllocator& allocator)
	{
		QueueFamilyIndices indices;

		std::vector<vk::QueueFamilyProperties, spite::HeapAllocator> queueFamilies = physicalDevice.
			getQueueFamilyProperties<spite::HeapAllocator>(allocator);

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				indices.graphicsFamily = i;
			}
			else if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
			{
				indices.transferFamily = i;
			}

			vk::Bool32 presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface).value;
			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}
			i++;
		}
		if (!indices.transferFamily.has_value())
		{
			indices.transferFamily.value() = indices.graphicsFamily.value();
		}

		return indices;
	}


	SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR surface)
	{
		SwapchainSupportDetails details;
		details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface).value;
		details.formats = physicalDevice.getSurfaceFormatsKHR(surface).value;
		details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface).value;
		return details;
	}


	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == vk::Format::eB8G8R8A8Srgb &&
				format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return format;
			}
		}
		return availableFormats[0];
	}

	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
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

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, int width, int height)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<u32>::max())
		{
			return capabilities.currentExtent;
		}
		vk::Extent2D actualExtent(static_cast<u32>(width),
		                          static_cast<u32>(height));
		actualExtent.width = std::clamp(actualExtent.width,
		                                capabilities.minImageExtent.width,
		                                capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height,
		                                 capabilities.minImageExtent.height,
		                                 capabilities.maxImageExtent.height);
		return actualExtent;
	}

	vk::SwapchainKHR createSwapchain(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface,
	                                 const SwapchainSupportDetails& swapchainSupport, const vk::Device& device,
	                                 const vk::Extent2D& extent, const vk::SurfaceFormatKHR& surfaceFormat,
	                                 const vk::PresentModeKHR& presentMode, const spite::HeapAllocator& allocator,
	                                 const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		u32 imageCount = swapchainSupport.capabilities.minImageCount + 1;

		if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount >
			swapchainSupport.capabilities.maxImageCount)
		{
			imageCount = swapchainSupport.capabilities.maxImageCount;
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
		                                      swapchainSupport.capabilities.currentTransform,
		                                      vk::CompositeAlphaFlagBitsKHR::eOpaque,
		                                      presentMode,
		                                      vk::True);


		QueueFamilyIndices indices = findQueueFamilies(surface, physicalDevice, allocator);
		u32 queueFamilyInidces[] = {
			indices.graphicsFamily.value(), indices.presentFamily.value()
		};

		if (indices.graphicsFamily != indices.presentFamily)
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

	std::vector<vk::Image> getSwapchainImages(const vk::Device& device, const vk::SwapchainKHR& swapchain)
	{
		auto [result,swapchainImages] = device.getSwapchainImagesKHR(swapchain);
		SASSERT_VULKAN(result)
		return swapchainImages;
	}

	eastl::vector<vk::ImageView, spite::HeapAllocator> createImageViews(const vk::Device& device,
	                                                                    const std::vector<vk::Image>& swapchainImages,
	                                                                    const vk::Format& imageFormat,
	                                                                    const vk::AllocationCallbacks*
	                                                                    pAllocationCallbacks)
	{
		eastl::vector<vk::ImageView, spite::HeapAllocator> imageViews;
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

	vk::RenderPass createRenderPass(const vk::Device& device, const vk::Format& imageFormat,
	                                const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::AttachmentDescription colorAttachment(
			{},
			imageFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR);

		vk::AttachmentReference colorAttachmentRef(
			0,
			vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription supbpass({},
		                                vk::PipelineBindPoint::eGraphics,
		                                {},
		                                {},
		                                1,
		                                &colorAttachmentRef);

		vk::RenderPassCreateInfo renderPassInfo({},
		                                        1,
		                                        &colorAttachment,
		                                        1,
		                                        &supbpass);

		vk::SubpassDependency dependency(vk::SubpassExternal,
		                                 0,
		                                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                 vk::PipelineStageFlagBits::eColorAttachmentOutput,
		                                 {},
		                                 vk::AccessFlagBits::eColorAttachmentWrite);

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		auto [result, renderPass] = device.createRenderPass(renderPassInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result);
		return renderPass;
	}

	vk::DescriptorSetLayout createDescriptorSetLayout(const vk::Device& device,
	                                                  const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::DescriptorSetLayoutBinding uboLayoutBinding(
			0,
			vk::DescriptorType::eUniformBuffer,
			1,
			vk::ShaderStageFlagBits::eVertex,
			{});

		vk::DescriptorSetLayoutCreateInfo layoutInfo(
			{},
			1,
			&uboLayoutBinding);

		auto [result, descriptorSetLayout] = device.createDescriptorSetLayout(layoutInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return descriptorSetLayout;
	}

	vk::ShaderModule createShaderModule(const vk::Device& device, const std::vector<char>& code,
	                                    const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::ShaderModuleCreateInfo createInfo(
			{},
			code.size(),
			reinterpret_cast<const u32*>(code.data()));

		auto [result, shaderModule] = device.createShaderModule(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return shaderModule;
	}

	//TODO: separate ShaderModule creation, add pipeline cache 
	vk::Pipeline createGraphicsPipeline(const vk::Device& device, const vk::DescriptorSetLayout& descriptorSetLayout,
	                                    const vk::Extent2D& swapchainExtent, const vk::RenderPass& renderPass,
	                                    const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		auto vertShaderCode = readBinaryFile("shaders/vert.spv");
		auto fragShaderCode = readBinaryFile("shaders/frag.spv");
		vk::ShaderModule vertShaderModule = createShaderModule(device, vertShaderCode, pAllocationCallbacks);
		vk::ShaderModule fragShaderModule = createShaderModule(device, fragShaderCode, pAllocationCallbacks);
		vertShaderCode.clear();
		fragShaderCode.clear();

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo(
			{},
			vk::ShaderStageFlagBits::eVertex,
			vertShaderModule,
			"main");

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo(
			{},
			vk::ShaderStageFlagBits::eFragment,
			fragShaderModule,
			"main");

		vk::PipelineShaderStageCreateInfo shaderStages[] = {
			vertShaderStageInfo, fragShaderStageInfo
		};

		eastl::array dynamicStates = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
			vk::DynamicState::eLineWidth
		};

		vk::PipelineDynamicStateCreateInfo dynamicState(
			{},
			static_cast<uint32_t>(dynamicStates.size()),
			dynamicStates.data());

		vk::VertexInputBindingDescription bindingDescription(0, sizeof(glm::vec2), vk::VertexInputRate::eVertex);
		vk::VertexInputAttributeDescription attributeDescription(0, 0, vk::Format::eR32G32Sfloat);
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
			{},
			1,
			&bindingDescription,
			1,
			&attributeDescription);


		vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
			{},
			vk::PrimitiveTopology::eTriangleList,
			vk::False);

		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(swapchainExtent.width),
		                      static_cast<float>(swapchainExtent.height),
		                      0.0f,
		                      1.0f);

		vk::Rect2D scissor({}, swapchainExtent);

		vk::PipelineViewportStateCreateInfo viewportState(
			{},
			1,
			&viewport,
			1,
			&scissor);

		vk::PipelineRasterizationStateCreateInfo rasterizer({},
		                                                    vk::False,
		                                                    vk::False,
		                                                    vk::PolygonMode::eFill,
		                                                    vk::CullModeFlagBits::eBack,
		                                                    vk::FrontFace::eClockwise,
		                                                    vk::False);

		vk::PipelineMultisampleStateCreateInfo multisampling(
			{},
			vk::SampleCountFlagBits::e1,
			vk::False);

		vk::PipelineColorBlendAttachmentState colorBlendAttachment(
			vk::False,
			{},
			{},
			{},
			{},
			{},
			{},
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA);

		vk::PipelineColorBlendStateCreateInfo colorBlending(
			{},
			vk::False,
			{},
			1,
			&colorBlendAttachment);

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
			{},
			1,
			&descriptorSetLayout);

		auto [result,pipelineLayout] = device.createPipelineLayout(pipelineLayoutInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result);

		vk::GraphicsPipelineCreateInfo pipelineInfo({},
		                                            2,
		                                            shaderStages,
		                                            &vertexInputInfo,
		                                            &inputAssembly,
		                                            {},
		                                            &viewportState,
		                                            &rasterizer,
		                                            &multisampling,
		                                            {},
		                                            &colorBlending,
		                                            &dynamicState,
		                                            pipelineLayout,
		                                            renderPass,
		                                            0);
		vk::Pipeline graphicsPipeline;
		std::tie(result, graphicsPipeline) =
			device.createGraphicsPipeline({}, pipelineInfo, pAllocationCallbacks);

		SASSERT_VULKAN(result)
		return graphicsPipeline;
	}

	eastl::vector<vk::Framebuffer, spite::HeapAllocator> createFramebuffers(const vk::Device& device,
	                                                                        const spite::HeapAllocator& allocator,
	                                                                        const eastl::vector<
		                                                                        vk::ImageView, spite::HeapAllocator>&
	                                                                        imageViews,
	                                                                        const vk::Extent2D& swapchainExtent,
	                                                                        const vk::RenderPass& renderPass,
	                                                                        const vk::AllocationCallbacks*
	                                                                        pAllocationCallbacks)
	{
		eastl::vector<vk::Framebuffer, spite::HeapAllocator> swapchainFramebuffers(allocator);
		swapchainFramebuffers.resize(imageViews.size());

		for (size_t i = 0; i < imageViews.size(); ++i)
		{
			vk::ImageView attachments[] = {imageViews[i]};

			vk::FramebufferCreateInfo framebufferInfo({},
			                                          renderPass,
			                                          1,
			                                          attachments,
			                                          swapchainExtent.width,
			                                          swapchainExtent.height,
			                                          1);

			vk::Result result;
			std::tie(result, swapchainFramebuffers[i]) = device.
				createFramebuffer(framebufferInfo, pAllocationCallbacks);
			SASSERT_VULKAN(result);
		}

		return swapchainFramebuffers;
	}

	vk::CommandPool createCommandPool(const vk::Device& device,
	                                  const vk::AllocationCallbacks* pAllocationCallbacks,
	                                  const vk::CommandPoolCreateFlags& flags, const u32 queueFamilyIndex)
	{
		vk::CommandPoolCreateInfo commandPoolCreateInfo(flags,
		                                                queueFamilyIndex);
		auto [result, commandPool] = device.createCommandPool(commandPoolCreateInfo, pAllocationCallbacks);
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
		u32 queues[] = {
			indices.graphicsFamily.value(),
			indices.transferFamily.value()
		};

		vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eConcurrent, 2, queues);

		vma::AllocationCreateInfo allocInfo(allocationFlags, vma::MemoryUsage::eAuto, properties);

		auto [result, bufferAllocation] = allocator.createBuffer(bufferInfo, allocInfo);
		SASSERT_VULKAN(result)

		buffer = bufferAllocation.first;
		bufferMemory = bufferAllocation.second;
	}

	void copyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, const vk::DeviceSize& size,
	                const vk::CommandPool& transferCommandPool, const vk::Device& device,
	                const vk::Queue& transferQueue, const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		vk::CommandBufferAllocateInfo allocInfo(transferCommandPool, vk::CommandBufferLevel::ePrimary, 1);

		auto [result,commandBuffers] = device.allocateCommandBuffers(allocInfo, pAllocationCallbacks);
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
	                                        vk::DescriptorType& type,
	                                        const u32 size)
	{
		vk::DescriptorPoolSize poolSize(type, size);
		vk::DescriptorPoolCreateInfo poolInfo(
			{},
			size,
			1,
			&poolSize);
		auto [result, descriptorPool] = device.createDescriptorPool(poolInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return descriptorPool;
	}

	std::vector<vk::DescriptorSet> createDescriptorSets(const vk::Device& device,
	                                                    const vk::DescriptorSetLayout& descriptorSetLayout,
	                                                    const vk::DescriptorPool& descriptorPool, sizet,
	                                                    const spite::HeapAllocator& allocator,
	                                                    const vk::AllocationCallbacks* pAllocationCallbacks,
	                                                    const u32 count)
	{
		eastl::vector<vk::DescriptorSetLayout, spite::HeapAllocator> layouts(
			count,
			descriptorSetLayout,
			allocator);
		vk::DescriptorSetAllocateInfo allocInfo(descriptorPool,
		                                        count,
		                                        layouts.data());

		auto [result, descriptorSets] = device.allocateDescriptorSets(allocInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return descriptorSets;
	}

	void updateDescriptorSets(const vk::Device& device, const vk::DescriptorSet& descriptorSet,
	                          const vk::Buffer& buffer, const vk::DescriptorType& type, const sizet bufferElementSize)
	{
		vk::DescriptorBufferInfo bufferInfo(buffer, 0, bufferElementSize);
		vk::WriteDescriptorSet descriptorWrite(descriptorSet,
		                                       0,
		                                       0,
		                                       1,
		                                       type,
		                                       {},
		                                       &bufferInfo,
		                                       {});

		device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
	}

	std::vector<vk::CommandBuffer> createGraphicsCommandBuffers(const vk::Device& device,
	                                                            const vk::CommandPool& graphicsCommandPool,
	                                                            const u32 count)
	{
		vk::CommandBufferAllocateInfo allocInfo(graphicsCommandPool,
		                                        vk::CommandBufferLevel::ePrimary,
		                                        count);

		auto [result, commandBuffers] = device.allocateCommandBuffers(allocInfo);
		SASSERT_VULKAN(result)

		return commandBuffers;
	}

	vk::Semaphore createSemaphore(const vk::Device device, const vk::SemaphoreCreateInfo& createInfo,
		const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		auto [result, semaphore] = device.createSemaphore(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return semaphore;
	}

	vk::Fence createFence(const vk::Device device, const vk::FenceCreateInfo& createInfo,
		const vk::AllocationCallbacks* pAllocationCallbacks)
	{
		auto [result, fence] = device.createFence(createInfo, pAllocationCallbacks);
		SASSERT_VULKAN(result)
		return fence;
	}

	void recordCommandBuffer(const vk::CommandBuffer& commandBuffer, const vk::Extent2D& swapchainExtent,
	                         const vk::RenderPass& renderPass, const vk::Framebuffer& framebuffer,
	                         const vk::Pipeline& graphicsPipeline, const vk::Buffer& buffer,
	                         const vk::DeviceSize& indicesOffset, const vk::PipelineLayout& pipelineLayout,
	                         const vk::DescriptorSet& descriptorSet, const u32 indexCount)
	{
		vk::CommandBufferBeginInfo beginInfo;
		vk::Result result = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(result)

		vk::Rect2D renderArea({}, swapchainExtent);
		vk::ClearValue clearColor({0.0f, 0.0f, 0.0f, 1.0f});
		vk::RenderPassBeginInfo renderPassInfo(renderPass,
		                                       framebuffer,
		                                       renderArea,
		                                       1,
		                                       &clearColor);

		commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		vk::Viewport viewport(0.0f,
		                      0.0f,
		                      static_cast<float>(swapchainExtent.width),
		                      static_cast<float>(swapchainExtent.height),
		                      0.0f,
		                      1.0f);
		commandBuffer.setViewport(0, 1, &viewport);


		commandBuffer.setScissor(0, 1, &renderArea);

		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &buffer, &offset);

		commandBuffer.bindIndexBuffer(buffer, indicesOffset,
		                              vk::IndexType::eUint16);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		                                 pipelineLayout,
		                                 0,
		                                 1,
		                                 &descriptorSet,
		                                 {},
		                                 {});

		commandBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
		commandBuffer.endRenderPass();

		result = commandBuffer.end();
		SASSERT_VULKAN(result)
	}

	vk::Result waitForFrame(const vk::Device& device, const vk::SwapchainKHR swapchain, const vk::Fence& inFlightFence,
	                        const vk::Semaphore& imageAvaliableSemaphore, const vk::CommandBuffer& commandBuffer,
	                        u32& imageIndex)
	{
		vk::Result result = device.waitForFences(1, &inFlightFence, vk::True, UINT64_MAX);
		SASSERT_VULKAN(result)

		result = device.resetFences({inFlightFence});
		SASSERT_VULKAN(result)

		result = commandBuffer.reset();
		SASSERT_VULKAN(result)

		result = device.acquireNextImageKHR(swapchain,
		                                    UINT64_MAX,
		                                    imageAvaliableSemaphore,
		                                    {}, &imageIndex);
		return result;
	}

	//command buffer has to be recorded before
	vk::Result drawFrame(const vk::CommandBuffer& commandBuffer, const vk::Fence& inFlightFence,
	                     const vk::Semaphore& imageAvaliableSemaphore, const vk::Semaphore& renderFinishedSemaphore,
	                     const vk::Queue& graphicsQueue, const vk::Queue& presentQueue,
	                     const vk::SwapchainKHR swapchain, const u32& imageIndex)
	{
		vk::Semaphore waitSemaphores[] = {imageAvaliableSemaphore};
		vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
		vk::Semaphore signalSemaphores[] = {
			renderFinishedSemaphore
		};


		vk::SubmitInfo submitInfo(1,
		                          waitSemaphores,
		                          waitStages,
		                          1,
		                          &commandBuffer,
		                          1,
		                          signalSemaphores);


		vk::Result result = graphicsQueue.submit({submitInfo}, inFlightFence);
		SASSERT_VULKAN(result)

		vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, &swapchain, &imageIndex, &result);
		result = presentQueue.presentKHR(presentInfo);

		return result;
	}
}
