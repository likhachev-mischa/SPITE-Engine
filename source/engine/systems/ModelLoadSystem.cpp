#include "ModelLoadSystem.hpp"

#include "base/File.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/RenderingComponents.hpp"
#include "engine/rendering/IRenderDevice.hpp"
#include "engine/rendering/IRenderResourceManager.hpp"

namespace spite
{
	void ModelLoadSystem::onInitialize(SystemContext ctx, SystemDependencyStorage& dependencyStorage)
	{
		setExecutionStage(CoreExecutionStages::UPDATE);
		auto& queryDesc = ctx.getQueryBuilder().with<Read<ModelLoadRequest>, Read<EventTag>>();

		requestQuery = registerQuery(queryDesc, dependencyStorage);
		setPrerequisite(requestQuery);
	}

	void ModelLoadSystem::onUpdate(SystemContext ctx)
	{
		auto& cb = ctx.getCommandBuffer();

		IRenderDevice* device;

		ctx.accessSingleton<RenderDeviceSingleton>([&device](const RenderDeviceSingleton& singleton)
		{
			device = singleton.renderDevice;
		});

		IRenderResourceManager* resourceManager;

		ctx.accessSingleton<RenderResourceManagerSingleton>(
			[&resourceManager](const RenderResourceManagerSingleton& singleton)
			{
				resourceManager = singleton.resourceManager;
			});

		for (const auto& request : requestQuery.view<Read<ModelLoadRequest>>())
		{
			printf("Loading model %s ...\n", request.filePath.c_str());
			auto allocMarker = FrameScratchAllocator::get().get_scoped_marker();
			auto vertices = makeScratchVector<Vertex>(FrameScratchAllocator::get());
			auto indices = makeScratchVector<u32>(FrameScratchAllocator::get());

			importModelAssimp(request.filePath.c_str(), vertices, indices);

			BufferHandle vertexBuffer = device->createBuffer({
				.size = sizeof(Vertex) * vertices.size(),
				.usage = BufferUsage::VERTEX_BUFFER | BufferUsage::TRANSFER_DST,
				.memoryUsage = MemoryUsage::GPU_ONLY
			});

			BufferHandle indexBuffer = device->createBuffer({
				.size = sizeof(u32) * indices.size(), .usage = BufferUsage::INDEX_BUFFER | BufferUsage::TRANSFER_DST,
				.memoryUsage = MemoryUsage::GPU_ONLY
			});

			resourceManager->updateBuffer(vertexBuffer, vertices.data(), sizeof(Vertex) * vertices.size());
			resourceManager->updateBuffer(indexBuffer, indices.data(), sizeof(u32) * indices.size());

			MeshComponent mesh;
			mesh.vertexBuffer = vertexBuffer;
			mesh.indexBuffer = indexBuffer;
			mesh.indexCount = static_cast<u32>(indices.size());

			Entity modelEntity = cb.createEntity();

			cb.addComponent(modelEntity, std::move(mesh));

			cb.addComponent(modelEntity, TransformComponent{});
			cb.addComponent(modelEntity, TransformRelationsComponent{});
			cb.addComponent(modelEntity, TransformMatrixComponent{});

			printf("Model %s loaded...\n", request.filePath.c_str());
		}
	}
}
