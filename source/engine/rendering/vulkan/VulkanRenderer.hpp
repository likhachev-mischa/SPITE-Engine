#pragma once
#include <memory>

#include <EASTL/array.h>

#include "VulkanRenderCommandBuffer.hpp"
#include "VulkanSwapchain.hpp"

#include "base/CollectionAliases.hpp"
#include "base/Platform.hpp"
#include "base/VulkanUsage.hpp"

#include "engine/rendering/GraphicsTypes.hpp"
#include "engine/rendering/IRenderer.hpp"
#include "engine/rendering/NamedBufferRegistry.hpp"
#include "engine/rendering/RenderGraph.hpp"
#include "engine/rendering/RenderResourceHandles.hpp"

namespace spite
{
	class VulkanSecondaryRenderCommandBuffer;
	struct VulkanRenderContext;
	class IRenderDevice;
	class WindowManager;
	class HeapAllocator;

	class VulkanRenderer : public IRenderer
	{
	private:
		friend class RenderGraph;

		std::unique_ptr<IRenderDevice> m_renderDevice;
		VulkanRenderContext& m_context;
		WindowManager& m_windowManager;
		RenderGraph* m_renderGraph = nullptr;
		VulkanSwapchain m_swapchain;

		NamedBufferRegistry m_namedBufferRegistry;

		// Frame management
		u32 m_currentFrame = 0;
		u32 m_currentSwapchainImageIndex = 0;
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;


		eastl::array<VulkanRenderCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers;
		eastl::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_imageAvailableSemaphores;
		heap_vector<vk::Semaphore> m_renderFinishedSemaphores;
		eastl::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> m_inFlightFences;

		vk::CommandPool m_commandPool; // For primary command buffers
		eastl::array<heap_unordered_map<heap_string, vk::CommandPool>, MAX_FRAMES_IN_FLIGHT> m_secondaryCommandPools;
		eastl::array<heap_unordered_map<heap_string, std::unique_ptr<VulkanSecondaryRenderCommandBuffer>>,
		             MAX_FRAMES_IN_FLIGHT> m_secondaryCommandBuffers;
		std::mutex m_poolCreationMutex;

		heap_vector<TextureHandle> m_swapchainTextureHandles;
		heap_vector<ImageViewHandle> m_swapchainImageViewHandles;
		heap_vector<vk::Fence> m_imagesInFlight;

		bool m_wasSwapchainRecreated = false;

		void recreateSwapchain();
		void createSwapchainResources();

	public:
		VulkanRenderer(VulkanRenderContext& context, WindowManager& windowManager, RenderGraph* renderGraph,
		               const HeapAllocator& allocator);

		~VulkanRenderer() override;

		VulkanRenderer(const VulkanRenderer&) = delete;
		VulkanRenderer& operator=(const VulkanRenderer&) = delete;
		VulkanRenderer(VulkanRenderer&& other) noexcept = delete;
		VulkanRenderer& operator=(VulkanRenderer&& other) noexcept = delete;

		void waitIdle() override;
		// --- Frame Lifecycle ---
		// Returns the primary command buffer for this frame.
		// Also resets all secondary command pools.
		// returns nullptr if swapchain was recreated
		IRenderCommandBuffer* beginFrame() override;
		void endFrameAndSubmit(IRenderCommandBuffer& commandBuffer) override;

		// --- Command Buffer Management ---
		ISecondaryRenderCommandBuffer* acquireSecondaryCommandBuffer(const heap_string& passName) override;

		void setRenderGraph(RenderGraph* renderGraph) override { m_renderGraph = renderGraph; }
		bool wasSwapchainRecreated() override;

		IRenderDevice& getDevice() override { return *m_renderDevice; }
		NamedBufferRegistry& getNamedBufferRegistry() override { return m_namedBufferRegistry; }
		ImageViewHandle getCurrentSwapchainImageView() const override;
		TextureHandle getCurrentSwapchainTextureHandle() const override;
		Format getSwapchainFormat() const override;
	};
}
