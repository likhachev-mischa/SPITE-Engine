#pragma once
#include "GraphicsApiManager.hpp"

namespace spite
{
	class IRenderCommandBuffer;

	class RenderingManager
	{
		GraphicsApiManager m_apiManager;
		IRenderCommandBuffer* m_cb{};

	public:
		RenderingManager(GraphicsApi api, WindowManager& windowManager, const HeapAllocator& allocator);

		void beginFrame();

		void render();

		const GraphicsApiManager& getApiManager();
	};
}
