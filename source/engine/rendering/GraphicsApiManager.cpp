#include "GraphicsApiManager.hpp"

#include <optional>

#include <SDL3/SDL_vulkan.h>

#include "RenderGraph.hpp"

#include "application/WindowManager.hpp"
#include "application/vulkan/VulkanWindowBinding.hpp"

#include "base/Assert.hpp"
#include "base/Logging.hpp"
#include "base/Math.hpp"

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

			RGResourceHandle positionHandle;
			RGResourceHandle normalHandle;
			RGResourceHandle albedoHandle;
			RGResourceHandle depthHandle;

			renderGraph.addPass<PassData>("Depth", [&](RGBuilder& builder, PassData& data)
			{
				depthHandle = builder.createTexture("DepthBuffer", depthDesc);

				builder.write(depthHandle, RGUsage::DepthStencilAttachmentWrite,
				              {
					              .loadOp = AttachmentLoadOp::CLEAR,
					              .storeOp = AttachmentStoreOp::STORE,
					              .clearValue = ClearDepthStencilValue{1.0f, 0}
				              });

				builder.useUbo("cameraUBO");

				PipelineDescription psoDesc{};
				psoDesc.depthTestEnable = true;
				psoDesc.depthWriteEnable = true;
				psoDesc.depthCompareOp = CompareOp::LESS;

				eastl::array shaders =
				{
					ShaderStageDescription{
						.path = "shaders/depthVert.spv", .stage = ShaderStage::VERTEX
					}
				};
				builder.setGraphicsPipeline(shaders, psoDesc);
			});

			renderGraph.addPass<PassData>("Geometry", [&](RGBuilder& builder, PassData& data)
			{
				positionHandle = builder.createTexture("PositionBuffer", gbufferFormatDesc);
				normalHandle = builder.createTexture("NormalBuffer", gbufferFormatDesc);
				albedoHandle = builder.createTexture("AlbedoBuffer", albedoDesc);

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

				builder.read(depthHandle, RGUsage::DepthStencilAttachmentWrite);

				builder.useUbo("cameraUBO");

				PipelineDescription psoDesc{};
				psoDesc.depthTestEnable = true;
				psoDesc.depthWriteEnable = false;
				psoDesc.depthCompareOp = CompareOp::EQUAL;
				psoDesc.blendStates = {{}, {}, {}};

				eastl::array shaders =
				{
					ShaderStageDescription{
						.path = "shaders/geometryVert.spv", .stage = ShaderStage::VERTEX
					},
					ShaderStageDescription{
						.path = "shaders/geometryFrag.spv", .stage = ShaderStage::FRAGMENT
					}
				};
				builder.setGraphicsPipeline(shaders, psoDesc);
			});


			renderGraph.addPass<PassData>("Light", [&](RGBuilder& builder, PassData& data)
			{
				// Declare reads from all G-Buffer textures and the depth buffer.
				// The graph uses this to ensure this pass runs after the Geometry Pass
				// and to set up the correct resource barriers.
				builder.read(positionHandle, RGUsage::FragmentShaderReadSampled);
				builder.read(normalHandle, RGUsage::FragmentShaderReadSampled);
				builder.read(albedoHandle, RGUsage::FragmentShaderReadSampled);
				builder.read(depthHandle, RGUsage::FragmentShaderReadSampled);

				// Declare the final write to the imported swapchain image.
				builder.writeToSwapchain(
					{
						.loadOp = AttachmentLoadOp::CLEAR,
						.clearValue = ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}
					});

				// Define the pipeline for this pass.
				PipelineDescription psoDesc{};
				psoDesc.depthTestEnable = false; // No depth testing needed
				psoDesc.depthWriteEnable = false;
				psoDesc.blendStates = {{}}; // One blend state for the final color attachment

				eastl::array shaders =
				{
					ShaderStageDescription{
						.path = "shaders/lightVert.spv", .stage = ShaderStage::VERTEX
					},
					ShaderStageDescription{
						.path = "shaders/lightFrag.spv", .stage = ShaderStage::FRAGMENT
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
