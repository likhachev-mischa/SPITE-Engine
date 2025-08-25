#include "VulkanRenderer.hpp"

#include <memory>
#include "VulkanRenderCommandBuffer.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanRenderDevice.hpp"
#include "VulkanSecondaryRenderCommandBuffer.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanTypeMappings.hpp"

#include "engine/rendering/RenderGraph.hpp"

#include "application/WindowManager.hpp"

#include "base/Assert.hpp"
#include "base/memory/HeapAllocator.hpp"

namespace spite
{
	VulkanRenderer::VulkanRenderer(VulkanRenderContext& context, WindowManager& windowManager, RenderGraph* renderGraph,
	                               const HeapAllocator& allocator)
		: m_renderDevice(std::make_unique<VulkanRenderDevice>(context, allocator)),
		  m_context(context),
		  m_windowManager(windowManager),
		  m_renderGraph(renderGraph),
		  m_namedBufferRegistry(*m_renderDevice),
		  m_renderFinishedSemaphores(makeHeapVector<vk::Semaphore>(allocator)),
		  m_swapchainTextureHandles(makeHeapVector<TextureHandle>(allocator)),
		  m_swapchainImageViewHandles(makeHeapVector<ImageViewHandle>(allocator))
	{
		for (sizet i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_secondaryCommandPools[i] = makeHeapMap<heap_string, vk::CommandPool>(allocator);
			m_secondaryCommandBuffers[i] = makeHeapMap<
				heap_string, std::unique_ptr<VulkanSecondaryRenderCommandBuffer>>(allocator);
		}

		SASSERT(m_renderDevice)

		int width, height;
		m_windowManager.getFramebufferSize(width, height);
		m_swapchain = VulkanSwapchain(m_context.physicalDevice, m_context.device, m_context.surface,
		                              width, height);

		createSwapchainResources();

		// Create primary command pool
		vk::CommandPoolCreateInfo poolInfo{};
		poolInfo.queueFamilyIndex = m_context.graphicsQueueFamily;
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

		auto [res, commandPool] = m_context.device.createCommandPool(poolInfo);
		SASSERT_VULKAN(res)
		m_commandPool = commandPool;

		eastl::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> nativeCbs;

		// Allocate primary command buffers
		vk::CommandBufferAllocateInfo allocInfo{};
		allocInfo.commandPool = m_commandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);

		res = m_context.device.allocateCommandBuffers(&allocInfo, nativeCbs.data());
		SASSERT_VULKAN(res)

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_commandBuffers[i] = VulkanRenderCommandBuffer(nativeCbs[i],
			                                                static_cast<VulkanRenderDevice*>(m_renderDevice.get()));
		}

		// Create sync objects
		vk::SemaphoreCreateInfo semaphoreInfo{};
		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			auto [res1, sem1] = m_context.device.createSemaphore(semaphoreInfo);
			SASSERT_VULKAN(res1)
			m_imageAvailableSemaphores[i] = sem1;
			auto [res3, fen] = m_context.device.createFence(fenceInfo);
			SASSERT_VULKAN(res3)
			m_inFlightFences[i] = fen;
		}
	}

	VulkanRenderer::~VulkanRenderer()
	{
		auto res = m_context.device.waitIdle();
		SASSERT_VULKAN(res)

		m_swapchain.destroy();

		// m_secondaryCommandBuffers are destroyed automatically via unique_ptr.
		// The vk::CommandBuffers they hold are freed when the pools are destroyed.
		for (const auto& poolMap : m_secondaryCommandPools)
		{
			for (const auto& pool : poolMap)
			{
				m_context.device.destroyCommandPool(pool.second);
			}
		}

		for (auto semaphore : m_renderFinishedSemaphores)
		{
			m_context.device.destroySemaphore(semaphore);
		}

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_context.device.destroySemaphore(m_imageAvailableSemaphores[i]);
			m_context.device.destroyFence(m_inFlightFences[i]);
		}

		m_context.device.destroyCommandPool(m_commandPool);
	}

	void VulkanRenderer::waitIdle()
	{
		auto res = m_context.device.waitIdle();
		SASSERT_VULKAN(res)
	}

	IRenderCommandBuffer* VulkanRenderer::beginFrame()
	{
		m_wasSwapchainRecreated = false;

		vk::Result res = m_context.device.waitForFences(1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
		SASSERT_VULKAN(res)

		res = m_context.device.acquireNextImageKHR(m_swapchain.get(), UINT64_MAX,
		                                           m_imageAvailableSemaphores[m_currentFrame], nullptr,
		                                           &m_currentSwapchainImageIndex);

		if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR)
		{
			recreateSwapchain();
			return nullptr;
		}

		SASSERT_VULKAN(res)

		res = m_context.device.resetFences(1, &m_inFlightFences[m_currentFrame]);
		SASSERT_VULKAN(res)

		// Reset all secondary command pools for the new frame.
		for (auto& pool : m_secondaryCommandPools[m_currentFrame])
		{
			res = m_context.device.resetCommandPool(pool.second, {});
			SASSERT_VULKAN(res)
		}

		// After the pools are reset, the command buffers are in the initial state.
		// We must also reset our C++ wrapper to reflect this.
		for (auto const& [passName, cmd] : m_secondaryCommandBuffers[m_currentFrame])
		{
			cmd->reset();
		}

		vk::CommandBuffer commandBuffer = m_commandBuffers[m_currentFrame].getNativeHandle();
		res = commandBuffer.reset();
		SASSERT_VULKAN(res)

		vk::CommandBufferBeginInfo beginInfo{};
		res = commandBuffer.begin(beginInfo);
		SASSERT_VULKAN(res)

#ifndef SPITE_USE_DESCRIPTOR_SETS
		auto& resourceSetManager = m_renderDevice->getResourceSetManager();

		const eastl::array<BufferHandle, 2> descriptorBuffers = {
			resourceSetManager.getDescriptorBuffer(DescriptorType::UNIFORM_BUFFER),
			resourceSetManager.getDescriptorBuffer(DescriptorType::SAMPLER)
		};
		m_commandBuffers[m_currentFrame].bindDescriptorBuffers(descriptorBuffers);
#endif


		return &m_commandBuffers[m_currentFrame];
	}

	ISecondaryRenderCommandBuffer* VulkanRenderer::acquireSecondaryCommandBuffer(const heap_string& passName)
	{
		auto& currentFrameCommandBuffers = m_secondaryCommandBuffers[m_currentFrame];
		auto it = currentFrameCommandBuffers.find(passName);

		VulkanSecondaryRenderCommandBuffer* cmdPtr;

		if (it != currentFrameCommandBuffers.end())
		{
			cmdPtr = it->second.get();
		}

		else
		{
			// Command buffer does not exist, create it
			vk::CommandPool pool;
			{
				std::lock_guard<std::mutex> lock(m_poolCreationMutex);
				auto& currentFramePools = m_secondaryCommandPools[m_currentFrame];
				auto poolIt = currentFramePools.find(passName);
				if (poolIt == currentFramePools.end())
				{
					vk::CommandPoolCreateInfo threadPoolInfo{};
					threadPoolInfo.queueFamilyIndex = m_context.graphicsQueueFamily;
					threadPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
					auto [result, newPool] = m_context.device.createCommandPool(threadPoolInfo);
					SASSERT_VULKAN(result)
					poolIt = currentFramePools.emplace(passName, newPool).first;
				}
				pool = poolIt->second;
			}

			vk::CommandBufferAllocateInfo allocInfo{};
			allocInfo.commandPool = pool;
			allocInfo.level = vk::CommandBufferLevel::eSecondary;
			allocInfo.commandBufferCount = 1;

			auto [res, vkCmd] = m_context.device.allocateCommandBuffers(allocInfo);
			SASSERT_VULKAN(res)

			auto newCmd = std::make_unique<VulkanSecondaryRenderCommandBuffer>(vkCmd[0], *m_renderDevice);
			cmdPtr = newCmd.get();
			currentFrameCommandBuffers[passName] = std::move(newCmd);
		}

		if (cmdPtr->isFresh())
		{
			SASSERT(m_renderGraph)

			const PassRenderingInfo info = m_renderGraph->getPassRenderingInfo(passName);
			cmdPtr->begin(info.colorAttachmentFormats, info.depthAttachmentFormat);
			cmdPtr->setViewportAndScissor(info.renderArea);

			const PipelineHandle pipelineHandle = m_renderGraph->getPass(passName)->getPipelineHandle();
			SASSERT(pipelineHandle.isValid())
			cmdPtr->bindPipeline(pipelineHandle);

#ifndef SPITE_USE_DESCRIPTOR_SETS
			auto& resourceSetManager = m_renderDevice->getResourceSetManager();

			const eastl::array<BufferHandle, 2> descriptorBuffers = {
				resourceSetManager.getDescriptorBuffer(DescriptorType::UNIFORM_BUFFER),
				resourceSetManager.getDescriptorBuffer(DescriptorType::SAMPLER)
			};
			cmdPtr->bindDescriptorBuffers(descriptorBuffers);

			auto& resourceSets = m_renderGraph->getPassResourceSets(passName);
			if (!resourceSets.empty())
			{
				auto layoutHandle = m_renderDevice->getPipelineCache().getPipelineLayoutHandle(pipelineHandle);

				auto marker = FrameScratchAllocator::get().get_scoped_marker();
				scratch_vector<u32> setIndices;
				scratch_vector<u64> offsets;

				for (sizet setIdx = 0; setIdx < resourceSets.size(); ++setIdx)
				{
					auto& setHandle = resourceSets[setIdx];
					auto& set = resourceSetManager.getSet(setHandle);
					setIndices.push_back(setIdx);
					offsets.push_back(set.getOffset());
				}
				cmdPtr->setDescriptorBufferOffsets(layoutHandle, 0, setIndices, offsets);
			}
#endif
		}

		return cmdPtr;
	}

	bool VulkanRenderer::wasSwapchainRecreated()
	{
		return m_wasSwapchainRecreated;
	}

	void VulkanRenderer::endFrameAndSubmit(IRenderCommandBuffer& commandBuffer)
	{
		auto cbHandle = static_cast<VulkanRenderCommandBuffer&>(commandBuffer).getNativeHandle();

		auto res = cbHandle.end();
		SASSERT_VULKAN(res)

		vk::SubmitInfo submitInfo{};
		vk::Semaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
		vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cbHandle;

		vk::Semaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentSwapchainImageIndex]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		res = m_context.graphicsQueue.submit(1, &submitInfo, m_inFlightFences[m_currentFrame]);
		SASSERT_VULKAN(res)

		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		vk::SwapchainKHR swapchains[] = {m_swapchain.get()};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &m_currentSwapchainImageIndex;

		vk::Result result = m_context.presentQueue.presentKHR(&presentInfo);

		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
		{
			recreateSwapchain();
		}
		else
		{
			SASSERT_VULKAN(result)
		}

		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanRenderer::recreateSwapchain()
	{
		auto res = m_context.device.waitIdle();
		SASSERT_VULKAN(res)

		int width, height;
		m_windowManager.getFramebufferSize(width, height);
		m_swapchain.recreate(m_context.physicalDevice, m_context.device, m_context.surface,
		                     width, height);

		static_cast<VulkanRenderDevice*>(m_renderDevice.get())->clearSwapchainDependentCaches();
		createSwapchainResources();
		m_wasSwapchainRecreated = true;
	}

	void VulkanRenderer::createSwapchainResources()
	{
		auto& resourceManager = static_cast<VulkanRenderResourceManager&>(m_renderDevice->getResourceManager());

		for (auto semaphore : m_renderFinishedSemaphores)
		{
			m_context.device.destroySemaphore(semaphore);
		}
		m_renderFinishedSemaphores.clear();

		for (auto handle : m_swapchainImageViewHandles)
		{
			resourceManager.destroyImageView(handle);
		}
		for (auto handle : m_swapchainTextureHandles)
		{
			resourceManager.destroyTexture(handle);
		}
		m_swapchainImageViewHandles.clear();
		m_swapchainTextureHandles.clear();

		const auto& swapchainImages = m_swapchain.getImages();
		const auto& swapchainImageViews = m_swapchain.getImageViews();

		TextureDesc textureDesc = {};
		textureDesc.width = m_swapchain.getExtent().width;
		textureDesc.height = m_swapchain.getExtent().height;
		textureDesc.format = static_cast<Format>(m_swapchain.getImageFormat());

		for (size_t i = 0; i < swapchainImages.size(); ++i)
		{
			TextureHandle textureHandle = resourceManager.registerExternalTexture(swapchainImages[i], textureDesc);
			m_swapchainTextureHandles.push_back(textureHandle);

			ImageViewHandle viewHandle = resourceManager.registerExternalImageView(swapchainImageViews[i]);
			m_swapchainImageViewHandles.push_back(viewHandle);
		}

		m_renderFinishedSemaphores.resize(swapchainImages.size());
		vk::SemaphoreCreateInfo semaphoreInfo{};
		for (size_t i = 0; i < swapchainImages.size(); i++)
		{
			auto [res, sem] = m_context.device.createSemaphore(semaphoreInfo);
			SASSERT_VULKAN(res)
			m_renderFinishedSemaphores[i] = sem;
		}
	}

	ImageViewHandle VulkanRenderer::getCurrentSwapchainImageView() const
	{
		return m_swapchainImageViewHandles[m_currentSwapchainImageIndex];
	}

	TextureHandle VulkanRenderer::getCurrentSwapchainTextureHandle() const
	{
		return m_swapchainTextureHandles[m_currentSwapchainImageIndex];
	}

	Format VulkanRenderer::getSwapchainFormat() const
	{
		return vulkan::from_vulkan_format(m_swapchain.getImageFormat());
	}

	u32 VulkanRenderer::getSwapchainImageCount() const
	{
		return static_cast<u32>(m_swapchain.getImages().size());
	}
}
