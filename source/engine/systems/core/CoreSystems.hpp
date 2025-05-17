#pragma once

#include "ecs/Core.hpp"
#include "ecs/Queries.hpp"
#include "ecs/World.hpp"

#include "engine/components/CoreComponents.hpp"
#include "engine/components/VulkanComponents.hpp"

namespace spite
{
	struct Vertex;

	class VulkanInitSystem : public SystemBase
	{
	public:
		void onInitialize() override;
	};

	class CameraCreateSystem : public SystemBase
	{
	public:
		void onInitialize() override;
	};

	class ModelLoadSystem : public SystemBase
	{
	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;

	private:
		void createModelBuffers(Entity entity,
		                        const eastl::vector<Vertex, HeapAllocator>& vertices,
		                        const eastl::vector<u32, HeapAllocator>& indices,
		                        MeshComponent& mesh);

		//hardcoded for now
		void assingVertexInput(VertexInputData& vertexInputData);
	};

	//creates named entites for ShaderComponents
	//name is filepath 
	class ShaderCreateSystem : public SystemBase
	{
		std::type_index m_modelLoadRequestType = typeid(ModelLoadRequest);

	public:
		void onInitialize() override;

		//TODO: parse .spirv to create descriptorset layout automatically
		void onUpdate(float deltaTime) override;

	private:
		Entity findExistingShader(const cstring path);

		Entity createShaderEntity(const cstring path, const vk::ShaderStageFlagBits& stage);

		void createDescriptors(const Entity shaderEntity, const vk::ShaderStageFlagBits stage);
	};

	class GeometryPipelineCreateSystem : public SystemBase
	{
		std::type_index m_pipelineCreateRequestType = typeid(PipelineCreateRequest);

		Query1<PipelineLayoutComponent>* m_pipelineLayoutQuery;
		Query1<PipelineComponent>* m_pipelineQuery;

	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;

	private:
		//assumes that shader entity == descriptorSetLayout entity
		Entity findCompatiblePipelineLayout(const ShaderReference& shaderRef);

		Entity createPipelineLayoutEntity(const ShaderReference& shaderRef);

		Entity findCompatiblePipeline(const Entity layoutEntity,
		                              const ShaderReference& shaderReference,
		                              const VertexInputComponent& vertexInput);

		Entity createPipelineEntity(const Entity layoutEntity,
		                            const VertexInputComponent& vertexInputComponent,
		                            const ShaderReference& shaderReference);
	};

	class TransformationMatrixSystem : public SystemBase
	{
		Query2<TransformComponent, TransformMatrixComponent>* m_query;

	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;

	private:
		glm::mat4 calculateModelMatrix(Entity entity, TransformComponent& transform);
	};

	class CameraMatricesUpdateSystem : public SystemBase
	{
		Query2<TransformComponent, CameraMatricesComponent>* m_query1;
		Query2<CameraDataComponent, CameraMatricesComponent>* m_query2;

	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;

	private:
		glm::mat4 createViewMatrix(const glm::vec3& position, const glm::quat& rotation);

		glm::mat4 createProjectionMatrix(CameraDataComponent& data, float aspect);
	};

	class CameraUboUpdateSystem : public SystemBase
	{
	public:
		void onUpdate(float deltaTime) override;
	};

	class WaitForFrameSystem : public SystemBase
	{
	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;
	};

	//TODO peredelat
	class DescriptorUpdateSystem : public SystemBase
	{
		Query2<DescriptorSetLayoutComponent, DescriptorSetsComponent>* m_descriptorQuery;

		Query1<UniformBufferComponent>* m_uboQuery;
		SharedQuery1<UniformBufferSharedComponent>* m_uboSharedQuery;

	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;
	};

	class DepthPassSystem : public SystemBase
	{
		Query1<MeshComponent>* m_modelQuery;

	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;
	};

	class GeometryPassSystem : public SystemBase
	{
		Query1<PipelineComponent>* m_pipelineQuery;
		Query2<MeshComponent, PipelineReference>* m_modelQuery;

	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;
	};

	class LightPassSystem : public SystemBase
	{
		void onInitialize() override;

	public:
		void onUpdate(float deltaTime) override;
	};

	class PresentationSystem : public SystemBase
	{
	private:
		//Query1<PipelineComponent>* m_pipelineQuery;
		//Query2<MeshComponent, PipelineReference>* m_modelQuery;

	public:
		void onInitialize() override;

		//TODO: CREATE DESCRIPTOR SETS FOR PIPELINE INDIVIDUALLY
		void onUpdate(float deltaTime) override;
	};

	class CleanupSystem : public SystemBase
	{
	public:
		void onInitialize() override;

		void onUpdate(float deltaTime) override;
	};
}
