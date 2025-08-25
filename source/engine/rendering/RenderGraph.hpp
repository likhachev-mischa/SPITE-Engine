#pragma once
#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include <EASTL/span.h>

#include "GraphicsDescs.hpp"
#include "IPipelineCache.hpp"
#include "IResourceSetManager.hpp"
#include "RenderGraphUsages.hpp"
#include "RenderResourceHandles.hpp"
#include "Synchronization.hpp"

#include "base/CollectionAliases.hpp"
#include "base/HashedString.hpp"
#include "base/StringInterner.hpp"

namespace spite
{
	class NamedBufferRegistry;
	class WindowManager;
	class ISecondaryRenderCommandBuffer;
	class IRenderer;
	struct ShaderStageDescription;
	class IRenderDevice;
	class IRenderCommandBuffer;


	class RGPass;
	class RGBuilder;
	class RenderGraph;

	// Describes how a written attachment should be handled in a render pass.
	struct RGWriteAttachmentInfo
	{
		AttachmentLoadOp loadOp = AttachmentLoadOp::DONT_CARE;
		AttachmentStoreOp storeOp = AttachmentStoreOp::STORE;
		// Only used if loadOp is CLEAR.
		std::optional<ClearValue> clearValue = std::nullopt;
	};

	struct PassRenderingInfo
	{
		sbo_vector<Format> colorAttachmentFormats;
		Format depthAttachmentFormat = Format::UNDEFINED;
		Rect2D renderArea;
	};

	// A record of a resource being written to in a pass, including attachment details.
	struct RGWriteAccess
	{
		RGResourceAccess resource;
		// Only relevant if the resource is a texture being used as a render target.
		RGWriteAttachmentInfo attachment;
	};

	class RGPass
	{
		friend class RenderGraph;
		friend class RGBuilder;

	public:
		template <typename PassData>
		const PassData& getData() const { return std::any_cast<const PassData&>(m_passData); }

		PipelineHandle getPipelineHandle() const { return m_pipelineHandle; }

		RGPass(HashedString name);

	private:
		HashedString m_name;

		std::any m_passData;

		std::function<void(RGBuilder&, std::any&)> m_setup;
		std::function<void(const std::any&, ISecondaryRenderCommandBuffer&, IRenderer*, PipelineLayoutHandle)>
		m_executeOld;
		std::function<void(IRenderCommandBuffer&)> m_execute;

		u32 m_inDegree = 0;
		bool m_isCulled = true;
		sbo_vector<RGPass*> m_successors{};
		sbo_vector<RGResourceAccess> m_reads{};
		sbo_vector<RGWriteAccess> m_writes{};
		sbo_vector<HashedString> m_usedUbos{};

		bool m_writesToSwapchain = false;

		// --- Pipeline State ---
		bool m_isGraphicsPass = false;
		PipelineDescription m_pipelineDescription{}; // User-provided base description
		sbo_vector<ShaderStageDescription> m_shaderStages{};
		PipelineHandle m_pipelineHandle{};

		// --- Dynamic Rendering State ---
		sbo_vector<Format> m_colorAttachmentFormats{};
		Format m_depthAttachmentFormat = Format::UNDEFINED;
		Rect2D m_renderArea{};
		sbo_vector<ClearValue> m_clearValues{}; // This will be populated during compile

		sbo_vector<ResourceSetHandle> m_resourceSets{};

		sbo_vector<HashedString> m_declaredUbos{};
	};

	class RenderGraph
	{
	private:
		friend class RGBuilder;
		friend class RGPass;

		struct RGResource
		{
			HashedString name;
			bool isImported = false;

			RGPass* writer = nullptr;
			sbo_vector<RGPass*> readers;

			GpuResourceHandle physicalHandle;
			std::variant<TextureDesc, BufferDesc> desc;
		};

		struct PhysicalResourceState
		{
			AccessFlags access = AccessFlags::NONE;
			ImageLayout layout = ImageLayout::UNDEFINED;
			PipelineStage stage = PipelineStage::TOP_OF_PIPE;
		};

		IRenderDevice& m_device;
		heap_vector<RGResource> m_resources;
		heap_vector<std::unique_ptr<RGPass>> m_passes;

		heap_vector<RGPass*> m_sortedPasses;
		heap_vector<sbo_vector<std::variant<MemoryBarrier2, BufferBarrier2, TextureBarrier2>>> m_passBarriers;
		heap_unordered_map<u32, PhysicalResourceState> m_physicalResourceStates;

		heap_unordered_map<HashedString, RGPass*> m_passLookup;
		heap_unordered_map<HashedString, RGResourceHandle> m_resourceNameLookup;
		heap_unordered_map<u32, ImageViewHandle> m_managedImageViews;

		RGResourceHandle createManagedTexture(HashedString name, const TextureDesc& desc);
		RGResourceHandle createManagedBuffer(HashedString name, const BufferDesc& desc);

	public:
		RenderGraph(const HeapAllocator& allocator, IRenderDevice& device);

		RenderGraph(const RenderGraph& other) = delete;
		RenderGraph(RenderGraph&& other) noexcept = delete;
		RenderGraph& operator=(const RenderGraph& other) = delete;
		RenderGraph& operator=(RenderGraph&& other) noexcept = delete;

		~RenderGraph();

		RGResourceHandle importTexture(HashedString name, TextureHandle handle, const TextureDesc& desc);
		RGResourceHandle importBuffer(HashedString name, BufferHandle handle, const BufferDesc& desc);

		RGPass* getPass(HashedString name);
		const RGPass* getPass(HashedString name) const;
		PassRenderingInfo getPassRenderingInfo(HashedString passName) const;

		const sbo_vector<ResourceSetHandle>& getPassResourceSets(HashedString passName);

		PipelineLayoutHandle getPipelineLayoutForPass(HashedString passName);

		template <typename PassData>
		RGPass& addPass(
			HashedString name,
			std::function<void(RGBuilder&, PassData&)> setup,
			std::function<void(IRenderCommandBuffer&)> execute = nullptr
		);

		void compile(NamedBufferRegistry& namedBufferRegistry, Format swapchainFormat, Extent2D swapchainExtent);
		void execute(IRenderCommandBuffer& cmd, IRenderer* renderer);
		void clear();
	};

	class RGBuilder
	{
	public:
		RGBuilder(RenderGraph& graph, RGPass& pass);

		RGResourceHandle createTexture(HashedString name, const TextureDesc& desc) const;
		RGResourceHandle createBuffer(HashedString name, const BufferDesc& desc) const;
		RGResourceHandle importBuffer(HashedString name, BufferHandle handle, const BufferDesc& desc) const;

		void useUbo(HashedString uboName) const;

		RGResourceHandle read(RGResourceHandle handle, const RGResourceUsage& usage) const;
		RGResourceHandle write(RGResourceHandle handle, const RGResourceUsage& usage,
		                       const RGWriteAttachmentInfo& attachmentInfo = {}) const;
		void writeToSwapchain(const RGWriteAttachmentInfo& attachmentInfo = {}) const;


		void setGraphicsPipeline(eastl::span<const ShaderStageDescription> shaderStages,
		                         const PipelineDescription& baseDescription) const;

	private:
		RenderGraph& m_graph;
		RGPass& m_pass;
	};

	template <typename PassData>
	RGPass& RenderGraph::addPass(
		HashedString name,
		std::function<void(RGBuilder&, PassData&)> setup,
		std::function<void(IRenderCommandBuffer&)> execute)
	{
		auto newPass = std::make_unique<RGPass>(name);
		RGPass& pass = *newPass;

		m_passLookup[name] = &pass;
		m_passes.emplace_back(std::move(newPass));

		pass.m_passData = PassData{};

		pass.m_setup = [setup](RGBuilder& builder, std::any& data)
		{
			setup(builder, std::any_cast<PassData&>(data));
		};

		pass.m_execute = std::move(execute);

		RGBuilder builder(*this, pass);
		pass.m_setup(builder, pass.m_passData);

		return pass;
	}
}
