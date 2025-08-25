#include "RenderingManager.hpp"

#include "IRenderCommandBuffer.hpp"
#include "IRenderer.hpp"
#include "RenderGraph.hpp"
#include "RenderResourceHandles.hpp"
#include "Synchronization.hpp"
#include "application/WindowManager.hpp"

#include "engine/ui/UIInspectorManager.hpp"

namespace spite
{
	RenderingManager::RenderingManager(GraphicsApi api, WindowManager& windowManager,
	                                   const HeapAllocator& allocator)
		: m_apiManager(api, windowManager, allocator)
	{
	}

	RenderingManager::~RenderingManager()
	{
		UIInspectorManager::shutdown();
	}

	void RenderingManager::beginFrame()
	{
		m_cb = m_apiManager.renderer()->beginFrame();

		if (m_apiManager.renderer()->wasSwapchainRecreated())
		{
			m_apiManager.recreateRenderGraph();
		}

		if (m_cb)
		{
			UIInspectorManager::get()->beginFrame();
		}
	}

	void RenderingManager::render()
	{
		if (!m_cb)
		{
			return;
		}

		auto currentSwapchainTexture = m_apiManager.renderer()->getCurrentSwapchainTextureHandle();

		// Transition swapchain image for rendering
		TextureBarrier2 initialBarrier = {
			.texture = currentSwapchainTexture,
			.srcStageMask = PipelineStage::TOP_OF_PIPE,
			.srcAccessMask = AccessFlags::NONE,
			.dstStageMask = PipelineStage::COLOR_ATTACHMENT_OUTPUT,
			.dstAccessMask = AccessFlags::COLOR_ATTACHMENT_WRITE,
			.oldLayout = ImageLayout::UNDEFINED,
			.newLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL
		};
		m_cb->pipelineBarrier({}, {}, {&initialBarrier, 1});

		m_apiManager.renderGraph()->execute(*m_cb, m_apiManager.renderer());

		// Transition swapchain image for presentation
		TextureBarrier2 presentBarrier = {
			.texture = currentSwapchainTexture,
			.srcStageMask = PipelineStage::COLOR_ATTACHMENT_OUTPUT,
			.srcAccessMask = AccessFlags::COLOR_ATTACHMENT_WRITE,
			.dstStageMask = PipelineStage::BOTTOM_OF_PIPE,
			.dstAccessMask = AccessFlags::NONE,
			.oldLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = ImageLayout::PRESENT_SRC
		};
		m_cb->pipelineBarrier({}, {}, {&presentBarrier, 1});

		m_apiManager.renderer()->endFrameAndSubmit(*m_cb);
	}

	const GraphicsApiManager& RenderingManager::getApiManager()
	{
		return m_apiManager;
	}
}
