#include "GraphicsApiManager.hpp"

#include <optional>

#include <SDL3/SDL_vulkan.h>

#include "RenderGraph.hpp"
#include "ShaderPaths.hpp"

#include "application/WindowManager.hpp"
#include "application/vulkan/VulkanWindowBinding.hpp"

#include "base/Assert.hpp"
#include "base/Logging.hpp"
#include "base/Math.hpp"

#include "engine/ui/UIInspectorManager.hpp"

#include "vulkan/VulkanRenderContext.hpp"
#include "vulkan/VulkanRenderer.hpp"

namespace spite
{
	namespace
	{
		void setupDefferedPipeline(RenderGraph& renderGraph, u32 width, u32 height)
		{
			renderGraph.clear();
			struct PassData
			{
			};

			// G-Buffer texture descriptions
			TextureDesc depthDesc =
			{
				.width = width, .height = height, .format = Format::D32_SFLOAT,
				.usage = TextureUsage::DEPTH_STENCIL_ATTACHMENT | TextureUsage::SAMPLED
			};

			TextureDesc gbufferFormatDesc =
			{
				.width = width, .height = height, .format = Format::R16G16B16A16_SFLOAT,
				.usage = TextureUsage::COLOR_ATTACHMENT | TextureUsage::SAMPLED
			};

			TextureDesc albedoDesc =
			{
				.width = width, .height = height, .format = Format::B8G8R8A8_SRGB,
				.usage = TextureUsage::COLOR_ATTACHMENT | TextureUsage::SAMPLED
			};

			// Offscreen texture description for scene + UI
			TextureDesc offscreenDesc =
			{
				.width = width, .height = height, .format = Format::B8G8R8A8_SRGB,
				.usage = TextureUsage::COLOR_ATTACHMENT | TextureUsage::SAMPLED
			};

			RGResourceHandle positionHandle;
			RGResourceHandle normalHandle;
			RGResourceHandle albedoHandle;
			RGResourceHandle depthHandle;
			RGResourceHandle sceneColorHandle;
			RGResourceHandle uiTextureHandle;

			//TODO check depthchecks correctness
			// Depth Pass
			renderGraph.addPass<PassData>(toHashedString("Depth"), [&](RGBuilder& builder, PassData& data)
			{
				depthHandle = builder.createTexture(toHashedString("DepthBuffer"), depthDesc);

				builder.write(depthHandle, RGUsage::DepthStencilAttachmentWrite,
				              {
					              .loadOp = AttachmentLoadOp::CLEAR,
					              .storeOp = AttachmentStoreOp::STORE,
					              .clearValue = ClearDepthStencilValue{1.0f, 0}
				              });

				builder.useUbo(toHashedString("cameraUBO"));

				PipelineDescription psoDesc{};
				psoDesc.depthTestEnable = true;
				psoDesc.depthWriteEnable = true;
				psoDesc.depthCompareOp = CompareOp::LESS;

				eastl::array shaders =
				{
					ShaderStageDescription{
						.path = toHashedString(SPITE_SHADER_PATH("depth.vert")), .stage = ShaderStage::VERTEX
					}
				};
				builder.setGraphicsPipeline(shaders, psoDesc);
			});

			// Geometry Pass
			renderGraph.addPass<PassData>(toHashedString("Geometry"), [&](RGBuilder& builder, PassData& data)
			{
				positionHandle = builder.createTexture(toHashedString("PositionBuffer"), gbufferFormatDesc);
				normalHandle = builder.createTexture(toHashedString("NormalBuffer"), gbufferFormatDesc);
				albedoHandle = builder.createTexture(toHashedString("AlbedoBuffer"), albedoDesc);

				builder.write(positionHandle, RGUsage::ColorAttachmentWrite,
				              {
					              .loadOp = AttachmentLoadOp::CLEAR,
				              });
				builder.write(normalHandle, RGUsage::ColorAttachmentWrite,
				              {
					              .loadOp = AttachmentLoadOp::CLEAR,
				              });
				builder.write(albedoHandle, RGUsage::ColorAttachmentWrite,
				              {
					              .loadOp = AttachmentLoadOp::CLEAR,
				              });

				builder.read(depthHandle, RGUsage::DepthStencilAttachmentRead);

				builder.useUbo(toHashedString("cameraUBO"));

				PipelineDescription psoDesc{};
				psoDesc.depthTestEnable = true;
				psoDesc.depthWriteEnable = false;
				psoDesc.depthCompareOp = CompareOp::EQUAL;
				psoDesc.blendStates = {{}, {}, {}};

				eastl::array shaders =
				{
					ShaderStageDescription{
						.path = toHashedString(SPITE_SHADER_PATH("geometry.vert")), .stage = ShaderStage::VERTEX
					},
					ShaderStageDescription{
						.path = toHashedString(SPITE_SHADER_PATH("geometry.frag")), .stage = ShaderStage::FRAGMENT
					}
				};
				builder.setGraphicsPipeline(shaders, psoDesc);
			});

			// Light Pass (renders to offscreen texture)
			renderGraph.addPass<PassData>(toHashedString("Light"), [&](RGBuilder& builder, PassData& data)
			{
				builder.read(positionHandle, RGUsage::FragmentShaderReadSampled);
				builder.read(normalHandle, RGUsage::FragmentShaderReadSampled);
				builder.read(albedoHandle, RGUsage::FragmentShaderReadSampled);
				builder.read(depthHandle, RGUsage::FragmentShaderReadSampled);

				sceneColorHandle = builder.createTexture(toHashedString("SceneColor"), offscreenDesc);
				builder.write(sceneColorHandle, RGUsage::ColorAttachmentWrite,
				              {
					              .loadOp = AttachmentLoadOp::CLEAR,
					              .clearValue = ClearColorValue{0.1f, 0.1f, 0.1f, 1.0f}
				              });

				PipelineDescription psoDesc{};
				psoDesc.depthTestEnable = false;
				psoDesc.depthWriteEnable = false;
				psoDesc.cullMode = CullMode::NONE;
				psoDesc.blendStates = {{}};

				eastl::array shaders =
				{
					ShaderStageDescription{
						.path = toHashedString(SPITE_SHADER_PATH("light.vert")), .stage = ShaderStage::VERTEX
					},
					ShaderStageDescription{
						.path = toHashedString(SPITE_SHADER_PATH("light.frag")), .stage = ShaderStage::FRAGMENT
					}
				};
				builder.setGraphicsPipeline(shaders, psoDesc);
			});

			// UI Pass (renders to offscreen texture)
			renderGraph.addPass<PassData>(toHashedString("UI"), [&](RGBuilder& builder, PassData& data)
			                              {
				                              builder.read(sceneColorHandle, RGUsage::FragmentShaderReadSampled);
				                              // Dependency

				                              uiTextureHandle = builder.createTexture(
					                              toHashedString("UIColor"), offscreenDesc);
				                              builder.write(uiTextureHandle, RGUsage::ColorAttachmentWrite,
				                                            {
					                                            .loadOp = AttachmentLoadOp::CLEAR,
					                                            .clearValue = ClearColorValue{
						                                            0.0f, 0.0f, 0.0f, 0.0f
					                                            } // Clear to transparent
				                                            });

				                              PipelineDescription uiPipelineDesc{};
				                              uiPipelineDesc.blendStates.push_back({
					                              .blendEnable = true,
					                              .srcColorBlendFactor = BlendFactor::SRC_ALPHA,
					                              .dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA,
					                              .colorBlendOp = BlendOp::ADD,
					                              .srcAlphaBlendFactor = BlendFactor::ONE,
					                              .dstAlphaBlendFactor = BlendFactor::ZERO,
					                              .alphaBlendOp = BlendOp::ADD
				                              });
				                              uiPipelineDesc.depthTestEnable = false;
				                              uiPipelineDesc.depthWriteEnable = false;

				                              builder.setGraphicsPipeline({}, uiPipelineDesc);
			                              }, [](IRenderCommandBuffer& cmd) { UIInspectorManager::get()->render(cmd); });

			// Composite Pass (blends scene and UI to swapchain)
			renderGraph.addPass<PassData>(toHashedString("Composite"), [&](RGBuilder& builder, PassData& data)
			{
				builder.read(sceneColorHandle, RGUsage::FragmentShaderReadSampled);
				builder.read(uiTextureHandle, RGUsage::FragmentShaderReadSampled);

				builder.writeToSwapchain({.loadOp = AttachmentLoadOp::DONT_CARE});

				PipelineDescription psoDesc{};
				psoDesc.depthTestEnable = false;
				psoDesc.depthWriteEnable = false;
				psoDesc.cullMode = CullMode::NONE;
				psoDesc.blendStates = {{}};

				eastl::array shaders =
				{
					ShaderStageDescription{
						.path = toHashedString(SPITE_SHADER_PATH("light.vert")), .stage = ShaderStage::VERTEX
					},
					ShaderStageDescription{
						.path = toHashedString(SPITE_SHADER_PATH("composite.frag")), .stage = ShaderStage::FRAGMENT
					}
				};
				builder.setGraphicsPipeline(shaders, psoDesc);
			});
		}
	}

	GraphicsApiManager::GraphicsApiManager(GraphicsApi api, WindowManager& windowManager,
	                                       const HeapAllocator& allocator)
		: m_windowManager(windowManager), m_allocator(allocator)
	{
		switch (api)
		{
		case GraphicsApi::Vulkan:
			initVulkan();
			break;
		default:
			SASSERTM(false, "Unsupported graphics API")
		}

		m_renderGraph = m_allocator.new_object<RenderGraph>(m_allocator, m_renderer->getDevice());
		m_renderer->setRenderGraph(m_renderGraph);
		recreateRenderGraph();
	}

	GraphicsApiManager::~GraphicsApiManager()
	{
		m_renderer->waitIdle();

		m_allocator.delete_object(m_renderGraph);
		m_allocator.delete_object(m_renderer);
		m_allocator.delete_object(m_context);
	}

	void GraphicsApiManager::recreateRenderGraph() const
	{
		m_renderGraph->clear();
		int width, height;
		m_windowManager.getFramebufferSize(width, height);
		// Pass the registry to the setup function
		setupDefferedPipeline(*m_renderGraph, static_cast<u32>(width), static_cast<u32>(height));
		m_renderGraph->compile(m_renderer->getNamedBufferRegistry(), m_renderer->getSwapchainFormat(),
		                       {static_cast<u32>(width), static_cast<u32>(height)});
	}

	void GraphicsApiManager::initVulkan()
	{
		SDEBUG_LOG("Initializing Vulkan backend...\n")
		m_context = m_allocator.new_object<VulkanRenderContext>(
			static_cast<VulkanWindowBinding*>(m_windowManager.getBinding()));

		m_renderer = m_allocator.new_object<VulkanRenderer>(*static_cast<VulkanRenderContext*>(m_context),
		                                                    m_windowManager, nullptr, m_allocator);

		SDEBUG_LOG("Vulkan backend initialized\n")
	}
}
