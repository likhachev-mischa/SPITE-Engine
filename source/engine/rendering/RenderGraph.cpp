#include "RenderGraph.hpp"

#include <algorithm>
#include <queue>
#include <variant>

#include "IRenderCommandBuffer.hpp"
#include "IRenderDevice.hpp"
#include "IRenderer.hpp"
#include "IResourceSetLayoutCache.hpp"
#include "ISecondaryRenderCommandBuffer.hpp"
#include "NamedBufferRegistry.hpp"

#include "application/WindowManager.hpp"

#include "base/Assert.hpp"
#include "base/CollectionUtilities.hpp"
#include "base/Logging.hpp"

namespace spite
{
	RGPass::RGPass(heap_string name)
		: m_name(std::move(name))
	{
	}

	RenderGraph::RenderGraph(const HeapAllocator& allocator, IRenderDevice& device)
		: m_device(device), m_resources(makeHeapVector<RGResource>(allocator)),
		  m_passes(makeHeapVector<std::unique_ptr<RGPass>>(allocator)),
		  m_sortedPasses(makeHeapVector<RGPass*>(allocator)),
		  m_passBarriers(
			  makeHeapVector<sbo_vector<std::variant<MemoryBarrier2, BufferBarrier2, TextureBarrier2>>>(allocator)),
		  m_physicalResourceStates(makeHeapMap<u32, PhysicalResourceState>(allocator)),
		  m_passLookup(makeHeapMap<heap_string, RGPass*>(allocator)),
		  m_resourceNameLookup(makeHeapMap<heap_string, RGResourceHandle>(allocator)),
		  m_managedImageViews(makeHeapMap<u32, ImageViewHandle>(allocator))
	{
	}

	RenderGraph::~RenderGraph()
	{
		clear();
	}

	RGResourceHandle RenderGraph::importTexture(heap_string name, TextureHandle handle, const TextureDesc& desc)
	{
		u32 id = static_cast<u32>(m_resources.size());
		RGResource& res = m_resources.emplace_back();
		res.name = std::move(name);
		res.isImported = true;
		res.physicalHandle = handle;
		res.desc = desc;
		return {id};
	}

	RGResourceHandle RenderGraph::importBuffer(const heap_string& name, BufferHandle handle, const BufferDesc& desc)
	{
		auto it = m_resourceNameLookup.find(name);
		if (it != m_resourceNameLookup.end())
		{
			return it->second;
		}

		u32 id = static_cast<u32>(m_resources.size());
		RGResource& res = m_resources.emplace_back();
		res.name = name;
		res.isImported = true;
		res.physicalHandle = handle;
		res.desc = desc;

		RGResourceHandle rgHandle{id};
		m_resourceNameLookup[name] = rgHandle;
		return rgHandle;
	}

	RGResourceHandle RenderGraph::createManagedTexture(heap_string name, const TextureDesc& desc)
	{
		u32 id = static_cast<u32>(m_resources.size());
		RGResource& res = m_resources.emplace_back();
		res.name = std::move(name);
		res.isImported = false; // The key difference
		res.physicalHandle = {}; // Invalid handle initially
		res.desc = desc;
		return {id};
	}

	RGResourceHandle RenderGraph::createManagedBuffer(const heap_string& name, const BufferDesc& desc)
	{
		auto it = m_resourceNameLookup.find(name);
		if (it != m_resourceNameLookup.end())
		{
			return it->second;
		}

		u32 id = static_cast<u32>(m_resources.size());
		RGResource& res = m_resources.emplace_back();
		res.name = name;
		res.isImported = false;
		res.physicalHandle = {};
		res.desc = desc;

		RGResourceHandle rgHandle{id};
		m_resourceNameLookup[name] = rgHandle;
		return rgHandle;
	}

	RGPass* RenderGraph::getPass(const heap_string& name)
	{
		auto it = m_passLookup.find(name);
		if (it != m_passLookup.end())
		{
			return it->second;
		}
		return nullptr;
	}

	const RGPass* RenderGraph::getPass(const heap_string& name) const
	{
		auto it = m_passLookup.find(name);
		if (it != m_passLookup.end())
		{
			return it->second;
		}
		return nullptr;
	}

	PassRenderingInfo RenderGraph::getPassRenderingInfo(const heap_string& passName) const
	{
		const RGPass* pass = getPass(passName);
		SASSERTM(pass, "Pass %s not found", passName.c_str())

		return {
			.colorAttachmentFormats = pass->m_colorAttachmentFormats,
			.depthAttachmentFormat = pass->m_depthAttachmentFormat,
			.renderArea = pass->m_renderArea
		};
	}

	const sbo_vector<ResourceSetHandle>& RenderGraph::getPassResourceSets(const heap_string& passName)
	{
		const RGPass* pass = getPass(passName);
		SASSERT(pass)
		return pass->m_resourceSets;
	}

	PipelineLayoutHandle RenderGraph::getPipelineLayoutForPass(const heap_string& passName)
	{
		RGPass* pass = getPass(passName);
		SASSERTM(pass, "Pass %s not found", passName.c_str())

		const PipelineHandle pipelineHandle = pass->getPipelineHandle();
		SASSERTM(pipelineHandle.isValid(), "Pipeline is not valid for pass %s", passName.c_str())

		return m_device.getPipelineCache().getPipelineLayoutHandle(pipelineHandle);
	}

	void RenderGraph::compile(NamedBufferRegistry& namedBufferRegistry, Format swapchainFormat,
	                          Extent2D swapchainExtent)
	{
		// --- Build Dependency Graph & Topologically Sort ---
		for (auto& pass : m_passes)
		{
			pass->m_inDegree = 0;
			pass->m_successors.clear();
			pass->m_isCulled = false; // For now, no culling
		}

		scratch_unordered_map<u32, RGPass*> resourceWriters;
		for (auto& pass : m_passes)
		{
			for (const auto& write : pass->m_writes)
			{
				SASSERTM(resourceWriters.find(write.resource.handle.id) == resourceWriters.end(),
				         "Resource has multiple writers")
				resourceWriters[write.resource.handle.id] = pass.get();
			}
		}

		for (auto& pass : m_passes)
		{
			for (const auto& read : pass->m_reads)
			{
				auto it = resourceWriters.find(read.handle.id);
				if (it != resourceWriters.end())
				{
					RGPass* writerPass = it->second;
					if (writerPass != pass.get())
					{
						writerPass->m_successors.push_back(pass.get());
						pass->m_inDegree++;
					}
				}
			}
		}

		// --- Topological Sort (Kahn's Algorithm) ---
		m_sortedPasses.clear();
		std::queue<RGPass*> qSort;
		for (auto& pass : m_passes)
		{
			if (!pass->m_isCulled && pass->m_inDegree == 0)
			{
				qSort.push(pass.get());
			}
		}

		while (!qSort.empty())
		{
			RGPass* u = qSort.front();
			qSort.pop();
			m_sortedPasses.push_back(u);

			for (RGPass* v : u->m_successors)
			{
				v->m_inDegree--;
				if (v->m_inDegree == 0)
				{
					qSort.push(v);
				}
			}
		}

		sizet unculledCount = std::ranges::count_if(m_passes,
		                                            [](const auto& p) { return !p->m_isCulled; });
		SASSERTM(m_sortedPasses.size() == unculledCount, "Cycle detected in render graph or incorrect culling")

		SDEBUG_LOG("\n-- RenderGraph Compilation Started --\n")
		SDEBUG_LOG("Total Resources: %zu\n", m_resources.size())
		SDEBUG_LOG("Total Passes: %zu\n", m_passes.size())
		if (!m_sortedPasses.empty())
		{
			SDEBUG_LOG("Pass execution order:\n")
			for (const auto& pass : m_sortedPasses)
			{
				SDEBUG_LOG("  - %s\n", pass->m_name.c_str())
			}
		}

		// --- Physical Resource Creation for Managed Resources ---
		SDEBUG_LOG("\n--- Creating Physical Resources ---\n")
		for (u32 i = 0; i < m_resources.size(); ++i)
		{
			auto& resource = m_resources[i];
			if (!resource.isImported && !resource.physicalHandle.isValid())
			{
				if (std::holds_alternative<TextureDesc>(resource.desc))
				{
					const auto& desc = std::get<TextureDesc>(resource.desc);
					SDEBUG_LOG("Creating managed texture: '%s' (Handle: %u)\n", resource.name.c_str(), i)
					resource.physicalHandle = m_device.createTexture(desc);

					ImageViewDesc viewDesc{
						.texture = static_cast<TextureHandle>(resource.physicalHandle),
						.format = desc.format
					};
					m_managedImageViews[i] = m_device.createImageView(viewDesc);
				}
				else
				{
					const auto& desc = std::get<BufferDesc>(resource.desc);
					SDEBUG_LOG("Creating managed buffer: '%s' (Handle: %u)\n", resource.name.c_str(), i)
					resource.physicalHandle = m_device.createBuffer(desc);
				}
			}
			else if (resource.isImported)
			{
				SDEBUG_LOG("Using imported resource: '%s' (Handle: %u)\n", resource.name.c_str(), i)
			}
		}


		// --- Pass Setup, Barrier Generation, and Resource Set Allocation ---
		SDEBUG_LOG("\n--- Compiling Passes ---\n")
		for (u32 i = 0; i < m_sortedPasses.size(); ++i)
		{
			RGPass* pass = m_sortedPasses[i];
			SDEBUG_LOG("\nCompiling Pass: '%s'\n", pass->m_name.c_str())

			if (pass->m_isGraphicsPass)
			{
				// --- Determine Attachment Formats and Render Area for Dynamic Rendering ---
				pass->m_colorAttachmentFormats.clear();
				pass->m_depthAttachmentFormat = Format::UNDEFINED;
				SDEBUG_LOG("  Determining attachments:\n")

				bool renderAreaSet = false;
				if (!pass->m_writes.empty())
				{
					for (const auto& write : pass->m_writes)
					{
						const auto& resource = m_resources[write.resource.handle.id];
						if (std::holds_alternative<TextureDesc>(resource.desc))
						{
							const auto& texDesc = std::get<TextureDesc>(resource.desc);
							pass->m_renderArea.extent.width = texDesc.width;
							pass->m_renderArea.extent.height = texDesc.height;
							renderAreaSet = true;
							break; // Found a texture, we're done.
						}
					}
				}

				if (!renderAreaSet && pass->m_writesToSwapchain)
				{
					pass->m_renderArea.extent = swapchainExtent;
				}

				for (const auto& write : pass->m_writes)
				{
					RGResource& resource = m_resources[write.resource.handle.id];
					if (std::holds_alternative<TextureDesc>(resource.desc))
					{
						const auto& texDesc = std::get<TextureDesc>(resource.desc);
						if (has_flag(texDesc.usage, TextureUsage::DEPTH_STENCIL_ATTACHMENT))
						{
							pass->m_depthAttachmentFormat = texDesc.format;
							SDEBUG_LOG("    - Depth Attachment: %s\n", resource.name.c_str())
						}
						else
						{
							pass->m_colorAttachmentFormats.push_back(texDesc.format);
							SDEBUG_LOG("    - Color Attachment: %s'\n", resource.name.c_str())
						}
					}
				}
				if (pass->m_writesToSwapchain)
				{
					pass->m_colorAttachmentFormats.push_back(swapchainFormat);
					SDEBUG_LOG("    - Color Attachment: Swapchain\n")
				}

				// --- Create Pipeline ---
				if (!pass->m_shaderStages.empty())
				{
					SDEBUG_LOG("  Creating pipeline...\n")
					auto& pipelineCache = m_device.getPipelineCache();
					pass->m_pipelineDescription.renderPass = {}; // Invalid handle for dynamic rendering
					pass->m_pipelineDescription.colorAttachmentFormats = pass->m_colorAttachmentFormats;
					pass->m_pipelineDescription.depthAttachmentFormat = pass->m_depthAttachmentFormat;
					pass->m_pipelineHandle = pipelineCache.getOrCreatePipelineFromShaders(
						{pass->m_shaderStages.begin(), pass->m_shaderStages.end()},
						pass->m_pipelineDescription
					);
					SDEBUG_LOG("  Pipeline created (Handle: %u)\n", pass->m_pipelineHandle.id)

					auto& resourceSetManager = m_device.getResourceSetManager();
					const auto& finalPipelineDesc = pipelineCache.getPipelineDescription(pass->m_pipelineHandle);
					auto& resourceSetLayoutCache = m_device.getResourceSetLayoutCache();

					pass->m_resourceSets.clear();

					// Build a lookup map for the pass's reads by their symbolic name
					auto marker = FrameScratchAllocator::get().get_scoped_marker();
					auto readsByName = makeScratchMap<heap_string, const RGResourceAccess*>(
						FrameScratchAllocator::get());
					for (const auto& read : pass->m_reads)
					{
						readsByName[m_resources[read.handle.id].name] = &read;
					}

					SDEBUG_LOG("  Processing resource sets:\n")
					for (const auto& layoutHandle : finalPipelineDesc.resourceSetLayouts)
					{
						ResourceSetHandle setHandle = resourceSetManager.allocateSet(layoutHandle);
						pass->m_resourceSets.push_back(setHandle);
						SDEBUG_LOG("    Allocated ResourceSet (Handle: %u) for Layout (Handle: %u)\n", setHandle.id,
						           layoutHandle.id)

						sbo_vector<ResourceWrite> writes;
						const auto& layoutDesc = resourceSetLayoutCache.getLayoutDescription(layoutHandle);

						SDEBUG_LOG("    Processing bindings for Layout (Handle: %u):\n", layoutHandle.id)
						for (const auto& binding : layoutDesc.bindings)
						{
							SDEBUG_LOG("      - Binding %u: '%s' (Type: %d)\n", binding.binding, binding.name.c_str(),
							           static_cast<int>(binding.type))
							if (binding.type == DescriptorType::UNIFORM_BUFFER)
							{
								auto uboIt = std::ranges::find(pass->m_usedUbos, binding.name);
								if (uboIt != pass->m_usedUbos.end())
								{
									SDEBUG_LOG("        Creating/getting UBO: %s\n", binding.name.c_str())
									BufferDesc uboDesc{
										.size = binding.size,
										.usage = BufferUsage::UNIFORM_BUFFER | BufferUsage::TRANSFER_DST,
										.memoryUsage = MemoryUsage::GPU_ONLY
									};

									BufferHandle bufferHandle = namedBufferRegistry.getOrCreateBuffer(
										binding.name, uboDesc);

									writes.push_back({
										.binding = binding.binding, .type = binding.type, .resource = bufferHandle
									});
									SDEBUG_LOG("        Bound UBO '%s' to binding %u\n", binding.name.c_str(),
									           binding.binding)
								}
							}
							else
							{
								auto readIt = readsByName.find(binding.name);
								if (readIt != readsByName.end())
								{
									const RGResourceAccess* readAccess = readIt->second;
									RGResource& resource = m_resources[readAccess->handle.id];

									if (binding.type == DescriptorType::SAMPLED_TEXTURE)
									{
										const auto& texDesc = std::get<TextureDesc>(resource.desc);
										ImageViewHandle viewHandle = m_device.createImageView({
											.texture = static_cast<TextureHandle>(resource.physicalHandle),
											.format = texDesc.format
										});
										writes.push_back({
											.binding = binding.binding, .type = binding.type, .resource = viewHandle
										});
										SDEBUG_LOG("        Bound texture '%s' to binding %u\n", resource.name.c_str(),
										           binding.binding)
									}
									else if (binding.type == DescriptorType::STORAGE_BUFFER)
									{
										writes.push_back({
											.binding = binding.binding, .type = binding.type,
											.resource = resource.physicalHandle
										});
										SDEBUG_LOG("        Bound storage buffer '%s' to binding %u\n",
										           resource.name.c_str(), binding.binding)
									}
									// Add other types like SAMPLER, STORAGE_TEXTURE if needed
								}
							}
						}


						if (!writes.empty())
						{
							resourceSetManager.updateSet(setHandle, writes);
							SDEBUG_LOG("    Updated ResourceSet (Handle: %u) with %zu writes.\n", setHandle.id,
							           writes.size())
						}
					}
				}
			}
		}
		SDEBUG_LOG("\n--- RenderGraph Compilation Finished ---\n")
	}

	void RenderGraph::execute(IRenderCommandBuffer& cmd, IRenderer* renderer)
	{
		// Clear the state tracker at the beginning of execution.
		m_physicalResourceStates.clear();
		auto marker = FrameScratchAllocator::get().get_scoped_marker();

		for (u32 i = 0; i < m_sortedPasses.size(); ++i)
		{
			RGPass* pass = m_sortedPasses[i];

			// 1. Generate and record barriers for the upcoming pass
			{
				auto memoryBarriers = makeScratchVector<MemoryBarrier2>(FrameScratchAllocator::get());
				auto bufferBarriers = makeScratchVector<BufferBarrier2>(FrameScratchAllocator::get());
				auto textureBarriers = makeScratchVector<TextureBarrier2>(FrameScratchAllocator::get());

				auto processResourceUsage = [&](const RGResourceAccess& access)
				{
					RGResource& resource = m_resources[access.handle.id];
					if (!resource.physicalHandle.isValid()) return;

					u32 physicalId = resource.physicalHandle.id;
					auto& currentState = m_physicalResourceStates[physicalId];

					if (currentState.stage == PipelineStage::NONE)
					{
						currentState = {access.usage.access, access.usage.layout, access.usage.stage};
						return;
					}
					if (currentState.access == access.usage.access && currentState.layout == access.usage.layout &&
						currentState.stage == access.usage.stage)
					{
						return;
					}

					if (std::holds_alternative<TextureDesc>(resource.desc))
					{
						textureBarriers.push_back({
							.texture = static_cast<TextureHandle>(resource.physicalHandle),
							.srcStageMask = currentState.stage, .srcAccessMask = currentState.access,
							.dstStageMask = access.usage.stage, .dstAccessMask = access.usage.access,
							.oldLayout = currentState.layout, .newLayout = access.usage.layout
						});
					}
					else
					{
						bufferBarriers.push_back({
							.buffer = static_cast<BufferHandle>(resource.physicalHandle),
							.srcStageMask = currentState.stage, .srcAccessMask = currentState.access,
							.dstStageMask = access.usage.stage, .dstAccessMask = access.usage.access
						});
					}
					currentState = {access.usage.access, access.usage.layout, access.usage.stage};
				};

				for (const auto& read : pass->m_reads) { processResourceUsage(read); }
				for (const auto& write : pass->m_writes) { processResourceUsage(write.resource); }

				if (!memoryBarriers.empty() || !bufferBarriers.empty() || !textureBarriers.empty())
				{
					cmd.pipelineBarrier(memoryBarriers, bufferBarriers, textureBarriers);
				}
			}

			// Begin rendering
			ImageViewHandle depthAttachmentHandle;
			auto colorAttachmentHandles = makeScratchVector<ImageViewHandle>(FrameScratchAllocator::get());
			auto clearValues = makeScratchVector<ClearValue>(FrameScratchAllocator::get());

			if (pass->m_isGraphicsPass)
			{
				for (const auto& write : pass->m_writes)
				{
					const auto& resource = m_resources[write.resource.handle.id];
					if (std::holds_alternative<TextureDesc>(resource.desc))
					{
						const auto& texDesc = std::get<TextureDesc>(resource.desc);
						auto it = m_managedImageViews.find(write.resource.handle.id);
						SASSERT(it != m_managedImageViews.end())

						if (has_flag(texDesc.usage, TextureUsage::DEPTH_STENCIL_ATTACHMENT))
						{
							depthAttachmentHandle = it->second;
						}
						else
						{
							colorAttachmentHandles.push_back(it->second);
						}
						if (write.attachment.clearValue.has_value())
						{
							clearValues.push_back(write.attachment.clearValue.value());
						}
					}
				}
				if (pass->m_writesToSwapchain)
				{
					colorAttachmentHandles.push_back(renderer->getCurrentSwapchainImageView());
				}

				cmd.beginRendering(colorAttachmentHandles, depthAttachmentHandle, pass->m_renderArea, clearValues);
			}

			if (pass->m_execute)
			{
				pass->m_execute(cmd);
			}

			// Execute the pass's pre-recorded secondary command buffer
			if (pass->m_isGraphicsPass && pass->m_pipelineHandle.isValid())
			{
				ISecondaryRenderCommandBuffer* secCmd = renderer->acquireSecondaryCommandBuffer(pass->m_name);
				if (!secCmd->isFresh())
				{
					secCmd->end();
					cmd.executeCommands(secCmd);
				}
			}

			if (pass->m_isGraphicsPass)
			{
				cmd.endRendering();
			}
		}
	}

	RGBuilder::RGBuilder(RenderGraph& graph, RGPass& pass)
		: m_graph(graph), m_pass(pass)
	{
	}

	RGResourceHandle RGBuilder::createTexture(heap_string name, const TextureDesc& desc) const
	{
		return m_graph.createManagedTexture(std::move(name), desc);
	}

	RGResourceHandle RGBuilder::createBuffer(heap_string name, const BufferDesc& desc) const
	{
		return m_graph.createManagedBuffer(std::move(name), desc);
	}

	RGResourceHandle RGBuilder::importBuffer(heap_string name, BufferHandle handle, const BufferDesc& desc) const
	{
		return m_graph.importBuffer(std::move(name), handle, desc);
	}

	void RGBuilder::useUbo(const heap_string& uboName) const
	{
		m_pass.m_usedUbos.push_back(uboName);
	}

	RGResourceHandle RGBuilder::read(RGResourceHandle handle, const RGResourceUsage& usage) const
	{
		m_pass.m_reads.push_back({handle, usage});
		return handle;
	}

	RGResourceHandle RGBuilder::write(RGResourceHandle handle, const RGResourceUsage& usage,
	                                  const RGWriteAttachmentInfo& attachmentInfo) const
	{
		m_pass.m_writes.push_back({{handle, usage}, attachmentInfo});
		return handle;
	}

	void RGBuilder::writeToSwapchain(const RGWriteAttachmentInfo& attachmentInfo) const
	{
		m_pass.m_writesToSwapchain = true;
	}

	void RGBuilder::setGraphicsPipeline(eastl::span<const ShaderStageDescription> shaderStages,
	                                    const PipelineDescription& baseDescription) const
	{
		m_pass.m_isGraphicsPass = true;

		for (const auto& shaderStage : shaderStages)
		{
			m_pass.m_shaderStages.push_back(shaderStage);
		}

		m_pass.m_pipelineDescription = baseDescription;
	}

	void RenderGraph::clear()
	{
		// --- Destroy Managed Physical Resources ---
		for (auto& resource : m_resources)
		{
			if (!resource.isImported && resource.physicalHandle.isValid())
			{
				if (std::holds_alternative<TextureDesc>(resource.desc))
				{
					m_device.destroyTexture(static_cast<TextureHandle>(resource.physicalHandle));
				}
				else
				{
					m_device.destroyBuffer(static_cast<BufferHandle>(resource.physicalHandle));
				}
			}
		}

		m_resources.clear();
		m_passes.clear();
		m_sortedPasses.clear();
		m_passBarriers.clear();
		m_physicalResourceStates.clear();
		m_passLookup.clear();
		m_resourceNameLookup.clear();
	}
}
