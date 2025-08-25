#pragma once

#include "Base/Platform.hpp"
#include "GraphicsApi.hpp"
#include <memory>

struct SDL_Window;

namespace spite
{
    class IWindowApiBinding;

	class WindowManager
	{
	public:
		WindowManager(const WindowManager& other) = delete;
		WindowManager(WindowManager&& other) = delete;
		WindowManager& operator=(const WindowManager& other) = delete;
		WindowManager& operator=(WindowManager&& other) = delete;

		explicit WindowManager(GraphicsApi api);

		void terminate();

		void waitWindowExpand() const;

		void getFramebufferSize(int& width, int& height) const;

		[[nodiscard]] bool isMinimized() const;

		[[nodiscard]] bool shouldTerminate() const;

        [[nodiscard]] IWindowApiBinding* getBinding() const;

		[[nodiscard]] SDL_Window* getWindow() const;

		~WindowManager();

	private:
		SDL_Window* m_window{};
        std::unique_ptr<IWindowApiBinding> m_binding;
		bool m_shouldTerminate = false;

		void initWindow(u64 apiFlag);
	};
}
