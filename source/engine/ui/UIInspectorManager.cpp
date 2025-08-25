#include "UIInspectorManager.hpp"

#include <external/imgui/imgui.h>
#include <external/imgui/backends/imgui_impl_sdl3.h>
#include <external/imgui/backends/imgui_impl_vulkan.h>

#include "UIInspectors.hpp"

#include "application/WindowManager.hpp"

#include "engine/rendering/vulkan/VulkanRenderCommandBuffer.hpp"
#include "engine/rendering/vulkan/VulkanRenderContext.hpp"
#include "engine/rendering/vulkan/VulkanRenderDevice.hpp"
#include "engine/rendering/vulkan/VulkanRenderer.hpp"
#include "engine/rendering/vulkan/VulkanTypeMappings.hpp"

namespace spite
{
	UIInspectorManager* UIInspectorManager::s_instance = nullptr;

	void UIInspectorManager::init(WindowManager& windowManager, IRenderer& renderer, EntityManager& entityManager)
	{
		s_instance = new UIInspectorManager(windowManager, renderer, entityManager);

		s_instance->addInspector(inspectUIEntityHierchy);
		s_instance->addInspector(inspectUIEntityComponents);
	}

	void UIInspectorManager::shutdown()
	{
		delete s_instance;
		s_instance = nullptr;
	}

	UIInspectorManager* UIInspectorManager::get()
	{
		SASSERT(s_instance)
		return s_instance;
	}

	UIInspectorManager::UIInspectorManager(WindowManager& windowManager, IRenderer& renderer,
	                                       EntityManager& entityManager) :
		m_windowManager(windowManager),
		m_renderer(renderer),
		m_entityManager(entityManager)
	{
		// Downcast to get Vulkan-specific context and device for initialization
		auto& vkRenderer = static_cast<VulkanRenderer&>(renderer);
		auto& vkDevice = static_cast<VulkanRenderDevice&>(vkRenderer.getDevice());
		auto& vkContext = vkDevice.getContext();

		// 1. Setup ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		// 2. Setup Platform/Renderer backends
		ImGui_ImplSDL3_InitForVulkan(m_windowManager.getWindow());
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vkContext.instance;
		init_info.PhysicalDevice = vkContext.physicalDevice;
		init_info.Device = vkContext.device;
		init_info.Queue = vkContext.graphicsQueue;
		init_info.QueueFamily = vkContext.graphicsQueueFamily;
		init_info.DescriptorPool = VK_NULL_HANDLE; // Let the backend create its own pool
		init_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
		init_info.MinImageCount = vkRenderer.getSwapchainImageCount();
		init_info.ImageCount = vkRenderer.getSwapchainImageCount();
		init_info.UseDynamicRendering = true;

		// 3. Setup for dynamic rendering
		vk::Format color_attachment_format = vulkan::to_vulkan_format(vkRenderer.getSwapchainFormat());
		init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = reinterpret_cast<VkFormat*>(&
			color_attachment_format);

		ImGui_ImplVulkan_Init(&init_info);
		//
	}

	UIInspectorManager::~UIInspectorManager()
	{
		auto& vkRenderer = static_cast<VulkanRenderer&>(m_renderer);
		auto& vkDevice = static_cast<VulkanRenderDevice&>(vkRenderer.getDevice());
		auto& vkContext = vkDevice.getContext();

		vkDeviceWaitIdle(vkContext.device);
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL3_Shutdown();

		ImGui::DestroyContext();
	}

	void UIInspectorManager::addInspector(UIInspectorFn* inspector)
	{
		SASSERT(eastl::find(m_inspectors.begin(),m_inspectors.end(),inspector) == m_inspectors.end())

		m_inspectors.push_back(inspector);
	}

	void UIInspectorManager::setActive(bool status)
	{
		m_isEnabled = status;
	}

	bool UIInspectorManager::isActive() const
	{
		return m_isEnabled;
	}

	void UIInspectorManager::processEvent(const SDL_Event* event)
	{
		if (!m_isEnabled)
		{
			return;
		}

		ImGui_ImplSDL3_ProcessEvent(event);
	}

	void UIInspectorManager::beginFrame()
	{
		if (!m_isEnabled)
		{
			return;
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		draw();
	}

	void UIInspectorManager::render(IRenderCommandBuffer& cmd)
	{
		if (!m_isEnabled)
		{
			return;
		}

		auto& vkCmd = static_cast<VulkanRenderCommandBuffer&>(cmd);

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkCmd.getNativeHandle());
	}

	void UIInspectorManager::draw()
	{
		if (!m_isEnabled)
		{
			return;
		}

		for (UIInspectorFn* inspector : m_inspectors)
		{
			inspector(m_entityManager);
		}
	}
}
