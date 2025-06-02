#include "CoreSystems.hpp"

#include "base/File.hpp"
#include "base/Common.hpp"

namespace spite
{
	void ModelLoadSystem::onInitialize()
	{
		requireComponent(typeid(ModelLoadRequest));
	}

	void ModelLoadSystem::onUpdate(float deltaTime)
	{
		auto& requestTable = m_entityService->componentStorage()->getEventsAsserted<
			ModelLoadRequest>();


		for (auto& request : requestTable)
		{
			Entity entity;
			if (request.entity == Entity::undefined())
			{
				entity = m_entityService->entityManager()->createEntity();
				request.entity = entity;

				if (!request.name.empty())
				{
					m_entityService->entityManager()->setName(entity, request.name.c_str());
				}
			}
			else
			{
				entity = request.entity;
			}

			eastl::vector<Vertex, HeapAllocator> vertices(m_entityService->allocator());
			eastl::vector<u32, HeapAllocator> indices(m_entityService->allocator());
			importModelAssimp(request.objFilePath.c_str(), vertices, indices);
			//readModelInfoFile(request.objFilePath.c_str(),
			//                  vertices,
			//                  indices,
			//                  m_entityService->allocator());

			// Create mesh component
			MeshComponent mesh;
			createModelBuffers(entity, vertices, indices, mesh);
			m_entityService->componentManager()->addComponent<MeshComponent>(
				entity,
				std::move(mesh));

			//VertexInputData vertexInputData;
			//assingVertexInput(vertexInputData);

			//VertexInputComponent vertexInputComponent;
			//vertexInputComponent.vertexInputData = std::move(vertexInputData);
			//m_entityService->componentManager()->addComponent(
			//	entity,
			//	std::move(vertexInputComponent));

			m_entityService->componentManager()->addComponent<MovementDirectionComponent>(entity);
			m_entityService->componentManager()->addComponent<MovementSpeedComponent>(entity);

			if (!m_entityService->componentManager()->hasComponent(
				entity,
				typeid(TransformComponent)))
			{
				TransformComponent transform;
				m_entityService->componentManager()->addComponent<TransformComponent>(
					entity,
					transform);
			}

			if (!m_entityService->componentManager()->hasComponent(
				entity,
				typeid(TransformMatrixComponent)))
			{
				TransformMatrixComponent matrix;
				m_entityService->componentManager()->addComponent<TransformMatrixComponent>(
					entity,
					matrix);
			}

			//SDEBUG_LOG("MODEL LOADED\n")
		}
		m_entityService->entityEventManager()->rewindEvent(typeid(ModelLoadRequest));
	}

	void ModelLoadSystem::createModelBuffers(Entity entity,
		const eastl::vector<Vertex, HeapAllocator>& vertices,
		const eastl::vector<u32, HeapAllocator>& indices,
		MeshComponent& mesh)
	{
		mesh.indexCount = indices.size();

		auto deviceComponent = m_entityService->componentManager()->getSingleton<
			DeviceComponent>();
		auto gpuAllocatorComponent = m_entityService->componentManager()->getSingleton<
			GpuAllocatorComponent>();
		auto commandPoolComponent = m_entityService->componentManager()->getSingleton<
			CommandPoolComponent>();
		auto queueComponent = m_entityService->componentManager()->getSingleton<
			QueueComponent>();
		auto& allocationCallbacksComponent = m_entityService->componentManager()->getSingleton<
			AllocationCallbacksComponent>();

		sizet vertSize = vertices.size() * sizeof(vertices[0]);
		sizet indSize = indices.size() * sizeof(indices[0]);

		mesh.indexBuffer = BufferWrapper(indSize,
		                                 vk::BufferUsageFlagBits::eIndexBuffer |
		                                 vk::BufferUsageFlagBits::eTransferDst,
		                                 vk::MemoryPropertyFlagBits::eDeviceLocal,
		                                 {},
		                                 deviceComponent.queueFamilyIndices,
		                                 gpuAllocatorComponent.allocator);

		auto indexStage = BufferWrapper(indSize,
		                                vk::BufferUsageFlagBits::eTransferSrc |
		                                vk::BufferUsageFlagBits::eIndexBuffer,
		                                vk::MemoryPropertyFlagBits::eHostVisible |
		                                vk::MemoryPropertyFlagBits::eHostCoherent,
		                                vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
		                                | vma::AllocationCreateFlagBits::eStrategyMinTime,
		                                deviceComponent.queueFamilyIndices,
		                                gpuAllocatorComponent.allocator);

		//indexStage.copyMemory(vertices.data(), vertSize, 0);
		indexStage.copyMemory(indices.data(), indSize, 0);

		mesh.indexBuffer.copyBuffer(indexStage,
		                            deviceComponent.device,
		                            commandPoolComponent.transferCommandPool,
		                            queueComponent.transferQueue,
		                            &allocationCallbacksComponent.allocationCallbacks);

		mesh.vertexBuffer = BufferWrapper(vertSize,
		                                  vk::BufferUsageFlagBits::eVertexBuffer |
		                                  vk::BufferUsageFlagBits::eTransferDst,
		                                  vk::MemoryPropertyFlagBits::eDeviceLocal,
		                                  {},
		                                  deviceComponent.queueFamilyIndices,
		                                  gpuAllocatorComponent.allocator);

		auto vertexStage = BufferWrapper(vertSize,
		                                 vk::BufferUsageFlagBits::eTransferSrc |
		                                 vk::BufferUsageFlagBits::eVertexBuffer,
		                                 vk::MemoryPropertyFlagBits::eHostVisible |
		                                 vk::MemoryPropertyFlagBits::eHostCoherent,
		                                 vma::AllocationCreateFlagBits::eHostAccessSequentialWrite
		                                 | vma::AllocationCreateFlagBits::eStrategyMinTime,
		                                 deviceComponent.queueFamilyIndices,
		                                 gpuAllocatorComponent.allocator);

		vertexStage.copyMemory(vertices.data(), vertSize, 0);

		mesh.vertexBuffer.copyBuffer(vertexStage,
		                             deviceComponent.device,
		                             commandPoolComponent.transferCommandPool,
		                             queueComponent.transferQueue,
		                             &allocationCallbacksComponent.allocationCallbacks);
	}

	void ModelLoadSystem::assingVertexInput(VertexInputData& vertexInputData)
	{
		vertexInputData.bindingDescriptions.push_back({0, sizeof(Vertex)});

		vertexInputData.attributeDescriptions.reserve(3);
		vertexInputData.attributeDescriptions.push_back({0, 0, vk::Format::eR32G32B32Sfloat});
		vertexInputData.attributeDescriptions.push_back({1, 0, vk::Format::eR32G32B32Sfloat});
		vertexInputData.attributeDescriptions.push_back({2, 0, vk::Format::eR32G32Sfloat});
	}

}
