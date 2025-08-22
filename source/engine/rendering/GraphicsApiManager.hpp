#pragma once

#include "base/memory/HeapAllocator.hpp"

namespace spite
{
	struct IRenderContext;
	class IRenderDevice;
	class WindowManager;
	class RenderGraph;
	class IRenderer;
	enum class GraphicsApi;

	class GraphicsApiManager
	{
	private:
		IRenderContext* m_context{};
		IRenderer* m_renderer{};
		RenderGraph* m_renderGraph;

		WindowManager& m_windowManager;

		HeapAllocator m_allocator;

		void initVulkan();

	public:
		GraphicsApiManager(GraphicsApi api, WindowManager& windowManager, const HeapAllocator& allocator);
		~GraphicsApiManager();

		IRenderContext* renderContext() const { return m_context; }
		IRenderer* renderer() const { return m_renderer; }
		RenderGraph* renderGraph() const { return m_renderGraph; }

		void recreateRenderGraph() const;

		GraphicsApiManager(const GraphicsApiManager&) = delete;
		GraphicsApiManager& operator=(const GraphicsApiManager&) = delete;
		GraphicsApiManager(GraphicsApiManager&&) = delete;
		GraphicsApiManager& operator=(GraphicsApiManager&&) = delete;
	};
}
