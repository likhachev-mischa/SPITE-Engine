#pragma once

#include "base/CollectionAliases.hpp"

union SDL_Event;

namespace spite
{
	class IRenderer;
	class IRenderCommandBuffer;
	class WindowManager;
	class EntityManager;

	class UIInspectorManager
	{
	public:
		using UIInspectorFn = void(EntityManager&);

	private:
		UIInspectorManager(WindowManager& windowManager, IRenderer& renderer, EntityManager& entityManager);
		~UIInspectorManager();

		static UIInspectorManager* s_instance;

		bool m_isEnabled = true;
		WindowManager& m_windowManager;
		IRenderer& m_renderer;
		EntityManager& m_entityManager;

		heap_vector<UIInspectorFn*> m_inspectors;

	public:
		static void init(WindowManager& windowManager, IRenderer& renderer, EntityManager& entityManager);
		static void shutdown();
		static UIInspectorManager* get();

		UIInspectorManager(const UIInspectorManager&) = delete;
		UIInspectorManager& operator=(const UIInspectorManager&) = delete;
		UIInspectorManager(UIInspectorManager&& other) noexcept = delete;
		UIInspectorManager& operator=(UIInspectorManager&& other) noexcept = delete;

		void addInspector(UIInspectorFn* inspector);

		void setActive(bool status);
		bool isActive() const;
		void processEvent(const SDL_Event* event);
		void beginFrame();
		void render(IRenderCommandBuffer& cmd);
		void draw();
	};
}
